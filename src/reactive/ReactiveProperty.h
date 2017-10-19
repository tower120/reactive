#ifndef TEST_REACTIVEPROPERTY2_H
#define TEST_REACTIVEPROPERTY2_H

#include <reactive/details/ObservableProperty.h>
#include "observer.h"

namespace reactive{

	class ReactivePropertyBase {};

    template<class T, class blocking_class = reactive::default_blocking, bool t_threadsafe = true >
    class ReactiveProperty : ReactivePropertyBase {
	public:
		static constexpr const bool threadsafe = t_threadsafe;
	private:
        using Self = ReactiveProperty<T, blocking_class, threadsafe>;

		using DataBase = 
			std::conditional_t<threadsafe
				, details::ObservableProperty<T, blocking_class, Self>
				, details::ObservablePropertyConfigurable<T, blocking_class, Self, threading::dummy_mutex, threading::dummy_mutex, threading::dummy_mutex>
			>;

		struct DataNoLock {
			using Lock = typename DataBase::Lock;
			static constexpr const bool base_have_lock = true;
		};
		struct DataHaveLock {
			using Lock = threading::SpinLock<threading::SpinLockMode::Adaptive>;
		 	Lock reactiveproperty_lock;
			static constexpr const bool base_have_lock = false;
		};



		using DataLock = std::conditional_t<
			!threadsafe
			,DataNoLock		/* use threading::dummy_mutex from ObservablePropertyConfigurable */
			,std::conditional_t<
				std::is_same<typename DataBase::Lock, threading::dummy_mutex>::value
				, DataHaveLock
				, DataNoLock 
			>
		>;


        struct Data 
			: public DataBase
			, public DataLock
		{
            using Base = DataBase;
            using Base::operator=;
            using Base::Base;
			using typename DataLock::Lock;
			using Base::set_value;
			using Base::threadsafe;

			auto& get_mutex(std::true_type base_have_lock) {
				return Base::m_lock;
			}
			auto& get_mutex(std::false_type base_have_lock) {
				return DataLock::reactiveproperty_lock;
			}
			auto& get_mutex() {
				return get_mutex(std::integral_constant<bool, DataLock::base_have_lock>{});
			}

            std::function<void()> unsubscriber;
        };

        std::shared_ptr<Data> ptr;
    public:
		using Value = T;
        using WeakPtr   = std::weak_ptr<Data>;
        using SharedPtr = std::shared_ptr<Data>;
        using ReadLock  = typename Data::ReadLock;
        using WriteLock = typename Data::WriteLock;
		//using blocking_mode = typename DataBase::blocking_mode;


        ReactiveProperty()
            : ptr(std::make_shared<Data>()){};

        ReactiveProperty(const WeakPtr& weak)
            : ptr(weak.lock()){};

		ReactiveProperty(const SharedPtr& shared)
			: ptr(shared) {};
		ReactiveProperty(SharedPtr&& shared)
			: ptr(std::move(shared)) {};


        template<class any_mode>
        ReactiveProperty(const ObservableProperty<T, any_mode>& other)
            : ReactiveProperty(other.getCopy())
        {
            set_impl<false>([](const T& value) -> const T& {
                return value;
            }, other);
        }



        // in place ctr
        template<
            class Arg, class ...Args
            , class = typename std::enable_if_t<
                !(
                std::is_same< std::decay_t<Arg>, Self >::value
                || std::is_same< std::decay_t<Arg>, WeakPtr >::value
				|| std::is_same< std::decay_t<Arg>, SharedPtr >::value

				|| std::is_base_of<ObservablePropertyBase, std::decay_t<Arg>>::value
				|| std::is_base_of<ReactivePropertyBase, std::decay_t<Arg>>::value

                /*|| std::is_same< std::decay_t<Arg>, ObservableProperty<T, reactive::default_blocking> >::value
                || std::is_same< std::decay_t<Arg>, ObservableProperty<T, reactive::nonblocking> >::value
				|| std::is_same< std::decay_t<Arg>, ObservableProperty<T, reactive::nonblocking_atomic> >::value
                || std::is_same< std::decay_t<Arg>, ObservableProperty<T, reactive::blocking> >::value*/
                )
            >
        >
        ReactiveProperty(Arg&& arg, Args&&...args)
            : ptr(std::make_shared<Data>(std::forward<Arg>(arg), std::forward<Args>(args)...) ) {};


        // copy ctr, as listener
        ReactiveProperty(const ReactiveProperty& other)
            : ReactiveProperty(other.getCopy())
        {
            set_impl<false>([](const T& value) -> const T& {
                return value;
            }, other);
        }

    private:
        template<class Any>
        void set_assigment(const Any& any){
            set([](const T& value) -> const T& {
                return value;
            }, any);
        }

    public:
        ReactiveProperty& operator=(const ReactiveProperty& other){
            set_assigment(other);
            return *this;
        }
        template<class ...Any>
        ReactiveProperty& operator=(const ReactiveProperty<Any...>& other){
            set_assigment(other);
            return *this;
        }
        template<class ...Any>
        ReactiveProperty& operator=(const ObservableProperty<Any...>& other){
            set_assigment(other);
            return *this;
        }


		ReactiveProperty(ReactiveProperty&& other) noexcept
			:ptr(std::move(other.ptr)) {}
		ReactiveProperty& operator=(ReactiveProperty&& other) noexcept {
			ptr = std::move(other.ptr);
			return *this;
		}

    private:
        template<bool update_value = true, class set_blocking_mode = reactive::default_blocking, class Closure, class ...Observables>
        void set_impl(Closure&& closure, const Observables&... observables){
            std::unique_lock<typename DataLock::Lock> l(ptr->get_mutex());

            if (ptr->unsubscriber){
                ptr->unsubscriber();
            }

            auto observer = reactive::details::MultiObserver::observe_impl<set_blocking_mode>(
			[closure = std::forward<Closure>(closure), ptr_weak = std::weak_ptr<Data>(ptr)](auto&&...args){
                std::shared_ptr<Data> ptr = ptr_weak.lock();
                if(!ptr) return;

                (*ptr) = closure(std::forward<decltype(args)>(args)...);
            }, observables.shared_ptr()...);

            ptr->unsubscriber = [observer](){ observer->unsubscribe(); };

            if (update_value) {
                observer->execute([&](auto &&...args) {
                    ptr->set_value(closure(std::forward<decltype(args)>(args)...), std::move(l));
                });
            }
        }
    public:
        template<class set_blocking_mode = reactive::default_blocking, class Closure, class ...Observables>
        void set(Closure&& closure, const Observables&... observables){
            set_impl<true, set_blocking_mode>(std::forward<Closure>(closure), observables...);
        }


        template<class update_blocking_mode = reactive::default_blocking, class Closure, class ...Observables>
        void update(Closure&& closure, const Observables&... observables){
            std::unique_lock<typename DataLock::Lock> l(ptr->get_mutex());

            if (ptr->unsubscriber){
                ptr->unsubscriber();
            }

            auto observer = reactive::details::MultiObserver::observe_impl<update_blocking_mode>(
			[closure = std::forward<Closure>(closure), ptr_weak = std::weak_ptr<Data>(ptr)](auto&&...args){
                std::shared_ptr<Data> ptr = ptr_weak.lock();
                if(!ptr) return;

                auto write_ptr = ptr->write_lock();
                closure(write_ptr.get(), std::forward<decltype(args)>(args)...);
            }, observables.shared_ptr()...);

            ptr->unsubscriber = [observer](){ observer->unsubscribe(); };

            observer->execute([&](auto &&...args) {
                auto write_ptr = ptr->write_lock(std::move(l));
                closure(write_ptr.get(), std::forward<decltype(args)>(args)...);
            });
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


        template<class Delegate>
        void operator+=(Delegate&& closure) const {
            ptr->operator+=(std::forward<Delegate>(closure));
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
            std::unique_lock<typename DataLock::Lock> l(ptr->get_mutex());
            if (ptr->unsubscriber){
                ptr->unsubscriber();
            }
            ptr->set_value(value, std::move(l));
        }
        void operator=(T&& value) {
            std::unique_lock<typename DataLock::Lock> l(ptr->get_mutex());
            if (ptr->unsubscriber){
                ptr->unsubscriber();
            }
            ptr->set_value(std::move(value), std::move(l));
        }



        ReadLock lock() const {
            return ptr->lock();
        }
        WriteLock write_lock() {
            std::unique_lock<typename DataLock::Lock> l(ptr->get_mutex());
            if (ptr->unsubscriber){
                ptr->unsubscriber();
            }
            return ptr->write_lock(std::move(l));
        }
        T getCopy() const{
            return ptr->getCopy();
        }

		template<bool m_threadsafe = threadsafe, typename = std::enable_if_t<!m_threadsafe> >
		const T* operator->() const {
			return &(ptr->value);
		}
		template<bool m_threadsafe = threadsafe, typename = std::enable_if_t<!m_threadsafe> >
		const T& operator*() const {
			return ptr->value;
		}


        void pulse() const{
            ptr->pulse();
        }

        ~ReactiveProperty(){			
			if (!ptr) return; // moved?

            std::unique_lock<typename DataLock::Lock> l(ptr->get_mutex());
            if (ptr->unsubscriber){
                ptr->unsubscriber();
            }
        }

    };
}

#endif //TEST_REACTIVEPROPERTY2_H