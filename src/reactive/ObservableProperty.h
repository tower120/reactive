#ifndef REACTIVE_OBSERVABLEPROPERTY_H
#define REACTIVE_OBSERVABLEPROPERTY_H

#include "details/ObservableProperty.h"

namespace reactive{

	class ObservablePropertyBase {};

    // just wraps details::ObservableProperty with shared_ptr
    template<class T, class blocking_class = default_blocking, bool t_threadsafe = true>
    class ObservableProperty : ObservablePropertyBase {
	public:
		static constexpr const bool threadsafe = t_threadsafe;
	private:
        using Self = ObservableProperty<T, blocking_class, threadsafe>;

        using Property = std::conditional_t<threadsafe
			, details::ObservableProperty<T, blocking_class, Self>
			, details::ObservablePropertyConfigurable<T, blocking, Self, threading::dummy_mutex, threading::dummy_mutex, threading::dummy_mutex>
		>;
        std::shared_ptr<Property> ptr;
    public:
		using Value = T;
        using WeakPtr   = std::weak_ptr<Property>;
        using SharedPtr = std::shared_ptr<Property>;
        using ReadLock  = typename Property::ReadLock;
        using WriteLock = typename Property::WriteLock;

		//using blocking_mode = typename Property::blocking_mode;

        ObservableProperty()
            : ptr(std::make_shared<Property>()){};

        ObservableProperty(const WeakPtr& weak)
            : ptr(weak.lock()){};

		ObservableProperty(const SharedPtr& shared)
			: ptr(shared) {};
		ObservableProperty(SharedPtr&& shared)
			: ptr(std::move(shared)) {};


        template<
            class Arg, class ...Args
            , class = typename std::enable_if_t<
                    !(std::is_same< std::decay_t<Arg>, Self >::value
                        || std::is_same< std::decay_t<Arg>, WeakPtr >::value
						|| std::is_same< std::decay_t<Arg>, SharedPtr >::value
					)
                >
        >
        ObservableProperty(Arg&& arg, Args&&...args)
            : ptr(std::make_shared<Property>(std::forward<Arg>(arg), std::forward<Args>(args)...) ) {};


        ObservableProperty(ObservableProperty&&) = default;
        ObservableProperty& operator=(ObservableProperty&&) = default;

        ObservableProperty(const ObservableProperty& other)
            :ptr(std::make_shared<Property>(other.getCopy())) {}
        ObservableProperty& operator=(const ObservableProperty& other) {
            *ptr = *(other.ptr);
            return *this;
        }


        WeakPtr weak_ptr() const{
            return {ptr};
        }

        const SharedPtr& shared_ptr() const{
            return ptr;
        }

        operator bool() const{
            return ptr.operator bool();
        }

        // forward

        template<class DelegateT>
        void operator+=(DelegateT&& closure) const {
            ptr->operator+=(std::forward<DelegateT>(closure));
        }
        template<class Fn>
        void subscribe(const DelegateTag& tag, Fn&& fn) const {
            ptr->subscribe(tag, std::forward<Fn>(fn));
        }
        template<class Delegate>
        void operator-=(Delegate&& closure) const {
            ptr->operator-=(std::forward<Delegate>(closure));
        }



        void operator=(const T& value) {
            ptr->operator=(value);
        }
        void operator=(T&& value) {
            ptr->operator=(std::move(value));
        }

		template<bool m_threadsafe = threadsafe, typename = std::enable_if_t<!m_threadsafe> >
		const T* operator->() const {
			return &(ptr->value);
		}
		template<bool m_threadsafe = threadsafe, typename = std::enable_if_t<!m_threadsafe> >
		const T& operator*() const {
			return ptr->value;
		}

        ReadLock lock() const {
            return ptr->lock();
        }
        WriteLock write_lock() {
            return ptr->write_lock();
        }
        T getCopy() const{
            return ptr->getCopy();
        }

        void pulse() const{
            ptr->pulse();
        }
    };

}


#endif //REACTIVE_OBSERVABLEPROPERTY_H
