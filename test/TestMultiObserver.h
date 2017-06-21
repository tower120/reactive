#ifndef TEST_TESTMULTIOBSERVER_H
#define TEST_TESTMULTIOBSERVER_H

#include <reactive/observer.h>
#include <reactive/ObservableProperty.h>


/*
template <class T>
class DumReactiveProperty
        : public reactive::details::ObservableProperty<T>
        , public std::enable_shared_from_this<DumbReactiveProperty<T>>{

    std::atomic<std::function<void()>> unsubscriber;
public:
    template<class Closure, class ...Observables>
    void set(Closure&& closure, Observables&...observables){
        // atomic?
        std::unique_lock<Lock> l(m_lock());

        unsubscriber();

        // auto (ObserverNonBlocking)
        auto observer = reactive::details::subscribe_impl([closure, property_weak = property.weak_ptr()](auto&&...args){
            reactive::ObservableProperty<T> property{property_weak};
            if(!property) return;
            property = closure(args...);
        }, observables...);

        // update property
        //observer->execute();
    }

    ~DumReactiveProperty(){
        std::unique_lock<Lock> l(m_lock());
        if (unsubscriber) {
            unsubscriber();
        }
    }
};*/


class TestMultiObserver{
public:


    void test_simple(){
        reactive::ObservableProperty<int> x = 1;
        reactive::ObservableProperty<int> y = 2;


        reactive::ObservableProperty<int> summ;

        // variant 1
        std::function<void()>* unsub_ptr;
        std::function<void()> unsub = reactive::observe<reactive::nonblocking>([&](int x, int y){
            std::cout << x << " : " << y << std::endl;
            (*unsub_ptr)();
        }, x, y);
        unsub_ptr = &unsub;

        x = 10;
        x = 10;
        x = 9;
        x.pulse();
        unsub();
        x = 11;
    }

	void test_self_unsubscribe() {
		reactive::ObservableProperty<int> x = 1;
		reactive::ObservableProperty<int> y = 2;
		
		reactive::observe_w_unsubscribe([&](auto unsubscribe, int x, int y) {
			if (x == 10) unsubscribe();
			std::cout << x << " : " << y << std::endl;			
		}, x, y);

		x = 20;
		y = 40;

		x = 10;

		y = 100;

	}

    void test_all(){
        //test_simple();
		test_self_unsubscribe();
    }
};

#endif //TEST_TESTMULTIOBSERVER_H
