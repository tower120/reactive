Header only reactive C++ library. Thread-safe, memory-safe.
Concsists from:
* [Event](#event) (C# like `Event`, but thread-safe, does not block while event occurs, subscribtion/unsbuscription possible right from the observer )
* [ObservableProperty](#observableproperty)
* [ReactiveProperty](#reactiveproperty)

Helpers [observe](#observe) and [bind](#bind)

# Ussage
Add `src` folder to your compiler's INCLUDE path.

```C++
#include <reactive/ObservableProperty>
#include <reactive/ReactiveProperty>
#include <reactive/bind>

using namespace reactive;

ObservableProperty<int> x = 1;
ObservableProperty<int> y = 2;

ReactiveProperty<int> sum;
sum.set([](int x, int y){ return x+y; }, x, y);

sum += [](int sum){
    cout << "new sum is " << sum << endl;
};

struct MyWidget{
    void setText(const std::string& msg){
        cout << msg << endl;
    }
};
auto widget = std::make_shared<MyWidget>();     // need shared_ptr to keep track about widget aliveness in multithreaded environment

// non-intrusive bind
bind(widget, [](auto& widget, int sum, int x, int y){
    widget->setText(std::to_string(sum) + " = " + std::to_string(x) + " + " + std::to_string(y));
}, sum, x, y);

x = 10;
widget.reset();     // safe to kill, bind will take care about auto unsubscribing
y = 20;

```

# Event

Event is basis of reactivity. It let us know that something changed.

```C++
#include <reactive/Event.h>
using namespace reactive;

Event<int, int> mouseMove;

mouseMove += [](int x, int y){
    std::cout << "mouse position " << x << ":" << y << std::endl;
};

Delegate<int, int> delegate;
delegate = [&](int x, int y){    
    if (y == 100){        
        mouseMove -= delegate;        // it is possible to unsubscribe/subscrive right from the event;
    }
    std::cout << "delegate can be unsubscribed. Position " << x << ":" << y << std::endl;
};
mouseMove += delegate;      // delegate's function copied to event queue
mouseMove -= delegate;


// Delegate is shortcut for this:
DelegateTag tag;
mouseMove.subscribe(tag, [&, tag](int x, int y){
    if (x == 100){        
        mouseMove -= tag;   // You can store tag ( delegate.tag() ) for latter unsubscription
    }
    std::cout << "Event can be unsubscribed by Delegate tag too. Position " << x << ":" << y << std::endl;
});
mouseMove -= tag;

// call event
mouseMove(10,6);
```

#### `reactive/Event.h` Synopsis:  
`void operator()()`  
`void operator+=(Closure&&)`  
`void operator+=(const Delegate&)`  
`void operator-=(const Delegate&)`  
`void operator-=(const DelegateTag&)`  
`void subscribe(const DelegateTag&, Closure&&)`  

#### Implementation details:
Event use "deffered" container (see `details/utils/DefferedForwardContainer.h`), erase/emplace queued in separate std::vector, and applied before foreach(). Thus, foreach() have minimal interference with container modification. 

* subscription/unsubscription occurs before next event() call. 
* Subscription/unsubscription never blocked by event call();
* event call() does not block another event call(), if there is no subscription's/unsubscription's from previous call. Otherwise block till changes to event queue applied.
* Event queue is unordered.

### Delegate
```C++
struct Delegate{
    std::function
    DelegateTag m_tag

    auto& tag() { return m_tag; }
}
```

### DelegateTag
```C++
struct DelegateTag{
    unsigned log long int uuid;
}
```


# ObservableProperty

ObservableProperty = Value + Event

```C++
using namespace reactive;

struct Vec2{
    int x,y;
    Vec2() nothrow {}
    Vec2(int x, int y)
        :x(x)
        ,y(y)
    {}
}

ObservableProperty<Vec2> vec {10, 20};

vec += [](const Vec2& vec){
    std::cout << "new vec " << vec.x << ", " << vec.y << std::endl;
};

vec = {3,4};
// Output: new vec 3,4

{
    auto vec_ptr = vec.write_lock();
    vec_ptr->x = 2;
    vec_ptr->y = 5;
}
// Output: new vec 2,5

{
    auto vec_ptr = vec.lock();
    std::cout << "current vec " << vec_ptr->x << ", " << vec_ptr->y << std::endl;
}
// Output: current vec 2,5

std::thread t([](vec_weak = vec.weak_ptr()){
    ObservableProperty<Vec2> property(vec_weak);
    if (!property) return;

    Vec2 vec = property.getCopy();
    std::cout << "current vec " << vec.x << ", " << vec.y << std::endl;
});

```


If possible, on set, new value compares with previous, and event triggers only if values are not the same.
 

#### `reactive/ObservableProperty.h` Synopsis
**constructors**  
`ObservableProperty(Args&&...)`  in place object construct  
`ObservableProperty(const ObservableProperty&)` will copy only value  
`ObservableProperty(ObservableProperty&&)`  
`ObservableProperty(const WeakPtr&)`  may be invalid after construction. Check with bool()  
`ObservableProperty(const SharedPtr&)`  
**pointer**  
`WeakPtr weak_ptr() const`  
`const SharedPtr& shared_ptr() const`  
`operator bool() const`  
**event manipulation**  
`void operator+=(Closure&&) const`  
`void operator+=(const Delegate&) const`  
`void operator-=(const Delegate&) const`  
`void operator-=(const DelegateTag&) const`  
`void subscribe(const DelegateTag&, Closure&&) const`  
`void pulse() const`  
**accessors**  
`ReadLock lock() const`  
`T getCopy() const`  
**mutators**  
`WriteLock write_lock()`  
`void operator=(const T& value)`  
`void operator=(T&& value)`  
`ObservableProperty& operator=(const ObservableProperty&)` will copy only value 

---
> `lock()`/`write_lock()` lock object(with mutex, if applicable, see below), and provides pointer like object access. `WriteLock` will trigger event on destruction (aka update).
---

`ReadLock`/`WriteLock` Synopsis (`ReadLock` with `const` modifier):  
`T& get()`  
`operator T&()`  
`T* operator->()`  
`T& operator*()`  
`void unlock()` unlocks underlying mutex(if applicable) and call event loop with new value(for `WriteLock`)  
`void silent(bool be_silent = true)` does not call event on WriteLock destruction

`ObservableProperty` may be configured with additional parameter `ObservableProperty<T, blocking_mode>`.

Where `blocking_mode` can be:
 * `default_blocking` (by default).
 ``` 
   if (T is trivially copyable && size <= 128)  nonblocking_atomic
   if (T is copyable && size <= 128) nonblocking
   else blocking
```
 * `blocking` use `upgrade_mutex`. ReadLock use shared_lock. WriteLock use unique_lock. On setting new value, mutex locks with shared_lock, event called with value reference.
 * `nonblocking` use `SpinLock`. ReadLock copy value, does not use lock. WriteLock use unique_lock. On setting new value, event called with value copy (no locks).
 * `nonblocking_atomic` use `std::atomic<T>`. ReadLock copy value, does not use lock. WriteLock work with value copy, then atomically update property's value with it. On setting new value, event called with value copy (no locks).
 
 All in all, `blocking` never copy value, but lock internal mutex each time when you work with it. For small objects it is faster to copy, than lock, that's why `blocking` not used as default.

 Thoeretically, hardware supported std::atomic<T> with nonblocking_atomic should be the fastest. Keep in mind, that mostly, atomics are lockless for sizeof(T) <= 8.

Most of the time you will be happy with default. But, for containers, blocking mode preferable:
 ```C++
ObservableProperty< std::vector<int>, blocking >
```
Because all other modes, will make temporary copy of the vector.


#### Implementation details:
ObservableProperty internally holds shared_ptr. This needed to track alivness(and postpone destruction) in multithreaded environment.

Observable property consists from value and event. Event internally holds queue of observers, in heap allocated memory (std::vector) anyway. So shared_ptr construction overhead is not that big.

```C++
template<class T> struct ObservableProperty{
    struct Data{
        T value;
        Event<const T&> event;        
    };
    std::shared_ptr<Data> data;
}
```


# ReactiveProperty

Same as ObservableProperty + Can listen multiple ObservableProperties/ReactiveProperties and update value reactively.

```C++
using namespace reactive;

ObservableProperty<int> x = 1;
ObservableProperty<int> y = 3;

struct Vec2{
    int x,y;
    Vec2(int x, int y) :x(x) ,y(y){}
}
ReactiveProperty<Vec2> vec2 {0, 0};

vec2.set([](int x, int y){ return Vec2{x*x, y*y}; }, x, y);

vec2 += [](const Vec2& vec2){
    cout << "vec2 is " << vec2.x << ", " << vec2.y << endl;
};

x = 10;
// Output: vec2 is 100, 9

y = 2;
// Output: vec2 is 100, 4

vec2.update([](Vec2& vec2, int x, int y){ vec2.y = x+y; }, x, y);    // unsubscribe, and modify value
// Output: vec2 is 100, 12

x = 12;
// Output: vec2 is 100, 14

vec2 = {3,4};                       // unsubscribe, set value
// Output: vec2 is 3, 4

x = -2; // will not triger any changes in vec2
```

#### Synopsis  
same as ObservableProperty, except all mutators, first unsubscrbe previous listeners.  

`set<blocking_mode = default_blockign>(Closure&& closure, ObservableProperty/ReactiveProperty&...)`  
 observe properties, and call `closure` with values of properties (const Ts&...), result of `closure` set as current ReactiveProperty value. See  [observe](#observe).     
 ```C++
template<class T>
struct ReactiveProperty{
    T value;
    void set(Closure&& closure, Properties&&... properties){
        unsubscribe_previous();

        observe([closure, &value]( auto& ... values){
            value = closure(values...);
        }, properties);
    }
}
```


`update<blocking_mode = default_blockign>(Closure&&, ObservableProperty/ReactiveProperty&...)`  
observe properties, and call `closure` with first parameter as current value reference, and other as values of properties (const Ts&...). See  [observe](#observe).
```C++
template<class T>
struct ReactiveProperty{
    T value;
    void update(Closure&& closure, Properties&&... properties){
        unsubscribe_previous();

        observe([closure, &value]( auto& ... values){
            closure(value, values...);
        }, properties);
    }
}
```

`void operator=(const ObservableProperty/ReactiveProperty& property)` listen for property changes, and update self value with new one.  


# Observe
Allow observe multiple properties.
```C++
using namespace reactive;

ObservableProperty<int> x = 1;
ObservableProperty<int> y = 2;

auto unsubscribe = observe([](int x, int y){
    std::cout << x << ", " << y << std::endl;
}, x, y);

y = 4;
// Output: 1, 4

x = 5;
// Output: 5, 4

unsubscribe();
x = 8;
// trigger nothing


observe_w_unsubscribe([](auto unsubscribe, int x, int y){
    if (x == 100){
        unsubscribe();
        return;
    }
    
    std::cout << x << ", " << y << std::endl;
}, x, y);

x = 6;
// Output: 6, 4

x = 100;
y = 200;
// unsubscribed, trigger nothing
```

`observe`/`observe_w_unsubscribe` have optional `blocking_mode` template parameter:
```C++
template<class blocking_mode = default_blocking, class Closure, class ...Observables>
auto observe(Closure&&, Observables&...)
```

If blocking_mode == blocking, closure called with observables.lock()... If someone of observables dies, `observe` auto-unsubscribes.    
Otherwise, values stored in local tuple, and each time observables changes, tuple updates. Closure called with copy of that tuple. Thus, ommiting potential mutex lock on observables.lock()... If someone of observables dies, closure will be called with last known value of dead observable. Thus, it stop listen only when all observables dies.

Rules for default_blocking same as in [ObservableProperty](#observableproperty).


# Bind

`bind` designed for non-intrusive binding ObservableProperties/ReactivePropeties to non-aware class.
Class must be in `std::shared_ptr`. `bind` take care of observables and object lifetimes.  

```C++
using namespace reactive;

ObservableProperty<int> len = 2;
ObservableProperty<int> y{ 100 };

class Box {
    int m_len = 0;
public:

    auto len(int x) {
        m_len = x;
    }

    void show() {
        std::cout << m_len << std::endl;
    }
};

std::shared_ptr<Box> box = std::make_shared<Box>();

len = 40;

bind(box, [](auto box, int len) {
    box->len(len);
    box->show();
}, len);

bind_w_unsubscribe(box, [](auto unsubscibe, auto box, int len) {
    if (len > 100) unsubscibe();
    box->len(len);
    box->show();
}, len);

len = 50;
len = 101;
len = 60;
```

Stores object's weak_ptr, listen for observables. If object dies, unsubscribe self.

#### Synopsis
```C++
template<class blocking_mode = default_blocking, class Obj, class Closure, class ...Observables>
auto bind(const std::shared_ptr<Obj>& obj, Closure&& closure, const Observables&... observables)
// return unsubscriber
```

```C++	
template<class blocking_mode = default_blocking, class Obj, class Closure, class ...Observables>
auto bind_w_unsubscribe(const std::shared_ptr<Obj>& obj, Closure&& closure, const Observables&... observables)
// return unsubscriber
```


----
# Compiler support

Library should compiles with any standard compatible c++14/17 compiler.  
Tested with clang, gcc 6.3, vs 2017 c++

## VS2015-2017 , GCC < 7 
Objects in ObservableProperty, ReactiveProperty must be, also, no-throw default constructable in order to work in nonblocking_atomic mode
https://developercommunity.visualstudio.com/content/problem/69560/stdatomic-load-does-not-work-with-non-default-cons.html

in default_blocking mode, trivially constructable objects, but without no-throw default constructor, will work in nonblocking mode.