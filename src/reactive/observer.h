#ifndef MULTIOBSERVER_H
#define MULTIOBSERVER_H

#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>
#include <cassert>

#include "details/threading/SpinLock.h"
#include "details/Delegate.h"

#include "blocking.h"

namespace reactive{

    namespace details{
        namespace MultiObserver {
			// helper functions
			template<int i, class Closure>
			static void foreach(Closure &&closure) {}
            template<int i, class Closure, class Arg, class ...Args>
            static void foreach(Closure &&closure, Arg &&arg, Args &&... args) {
                closure(
                        std::integral_constant<int, i>{},
                        std::forward<Arg>(arg)
                );
                foreach<i + 1>(std::forward<Closure>(closure), std::forward<Args>(args)...);
            }
            template<class Closure, class ...Args>
            static void foreach(Closure &&closure, Args &&... args) {
                foreach<0>(std::forward<Closure>(closure), std::forward<Args>(args)...);
            }


			template<class F, class Tuple, std::size_t... I>
			static constexpr decltype(auto) apply_impl(F &&f, Tuple &&t, std::index_sequence<I...>) {
				return f(std::get<I>(std::forward<Tuple>(t))...);
			}
			template<class F, class Tuple>
			static constexpr decltype(auto) apply(F &&f, Tuple &&t) {
				return apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
					std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
			}

            template<class Closure, class Tuple>
            static void foreach_tuple(Closure &&closure, Tuple &&tuple) {
                reactive::details::MultiObserver::apply([&](auto &&... args) {
                    foreach(closure, std::forward<decltype(args)>(args)...);
                }, tuple);
            }

            template<class ...Objs, std::size_t ...Is>
            static auto lockWeakPtrs(const std::tuple<std::weak_ptr<Objs>...> &tuple, std::index_sequence<Is...>) {
                return std::make_tuple(std::get<Is>(tuple).lock()...);
            }

            template<class ...Objs>
            static auto lockWeakPtrs(const std::tuple<std::weak_ptr<Objs>...> &tuple) {
                return lockWeakPtrs(tuple, std::make_index_sequence<sizeof...(Objs)>{});
            }


            template<typename ... V>
            static constexpr bool and_all(const V &... v) {
                bool result = true;
                (void) std::initializer_list<int>{(result = result && v, 0)...};
                return result;
            }
			template<typename ... V>
			static constexpr bool or_all(const V &... v) {
				bool result = false;
				(void)std::initializer_list<int>{(result = result || v, 0)...};
				return result;
			}


            template<class Closure, class ...Observables>
            class ObserverBase {
            protected:
                bool unsubscribed = false;

                using Lock = threading::SpinLock<threading::SpinLockMode::Adaptive>;
                Lock lock;
            public:
                using ObservablesTuple = std::tuple<std::weak_ptr<Observables>...>;
                ObservablesTuple observable_weak_ptrs;

				DelegateTag tag;

                ObserverBase(const std::shared_ptr<Observables>&... observables)
                        : observable_weak_ptrs(observables...) {}

                void unsubscribe() {
                    std::unique_lock<Lock> l(lock);
                    if (unsubscribed) return;

                    foreach_tuple([&](auto i, auto &observable) {
                        auto ptr = observable.lock();
                        if (!ptr) return;

                        *ptr -= tag;
                    }, observable_weak_ptrs);

                    unsubscribed = true;
                }
            };

            template<bool add_unsubscibe_self, class Closure, class ...Observables>
            class ObserverBlocking : public ObserverBase<Closure, Observables...> {
                using Base = ObserverBase<Closure, Observables...>;
            public:
                using Base::observable_weak_ptrs;
                using Base::unsubscribe;

                std::decay_t<Closure> closure;

                template<class ClosureT>
                ObserverBlocking(ClosureT &&closure, const std::shared_ptr<Observables> &... observables)
                        : Base(observables...)
                        , closure(std::forward<ClosureT>(closure)) {}

                template<class IntegralConstant, class Arg>
                void run(IntegralConstant, Arg &&) {
                    execute();
                }

                void execute(){
                    execute(closure);
                }

            private:
                template<class ClosureT, class ObservableLocks>
                void apply_closure(ClosureT&& closure, ObservableLocks& observable_locks, std::false_type){
                    reactive::details::MultiObserver::apply([&](auto &... locks) {
                        closure(locks.get()...);
                    }, observable_locks);
                }
                template<class ClosureT, class ObservableLocks>
                void apply_closure(ClosureT&& closure, ObservableLocks& observable_locks, std::true_type){
                    reactive::details::MultiObserver::apply([&](auto &... locks) {
                        closure([&](){ unsubscribe(); }, locks.get()...);
                    }, observable_locks);
                }
            public:
                template<class ClosureT>
                void execute(ClosureT&& closure){
                    auto shared_ptrs = lockWeakPtrs(observable_weak_ptrs);
                    const bool all_locked = reactive::details::MultiObserver::apply([](auto &... ptrs) { return and_all(ptrs...); }, shared_ptrs);

                    if (!all_locked) {
                        // can't be reactive further, unsubscribe
                        unsubscribe();
                        return;
                    }

                    auto observable_locks = reactive::details::MultiObserver::apply([](auto &... observables) {
                        return std::make_tuple(observables->lock()...);
                    }, shared_ptrs);

                    apply_closure(std::forward<ClosureT>(closure), observable_locks, std::integral_constant<bool, add_unsubscibe_self>{});
                }
            };

            template<bool add_unsubscibe_self, class Closure, class ...Observables>
            class ObserverNonBlocking : public ObserverBase<Closure, Observables...> {
                using Base = ObserverBase<Closure, Observables...>;

                using Base::lock;
                using Lock = typename Base::Lock;
            public:
                using Base::unsubscribe;
                std::decay_t<Closure> closure;

                std::tuple<typename Observables::Value...> observable_values;

                template<class ClosureT>
                ObserverNonBlocking(ClosureT &&closure, const std::shared_ptr<Observables>&... observables)
                        : Base(observables...)
                        , closure(std::forward<ClosureT>(closure))
                        , observable_values(observables->getCopy()...) {}

            private:
                template<class ClosureT, class TmpValues>
                void apply_closure(ClosureT&& closure, const TmpValues& tmp_observable_values, std::false_type){
                    reactive::details::MultiObserver::apply(std::forward<ClosureT>(closure), tmp_observable_values);
                }
                template<class ClosureT, class TmpValues>
                void apply_closure(ClosureT&& closure, const TmpValues& tmp_observable_values, std::true_type){
                    reactive::details::MultiObserver::apply(
                        [&](auto&&...args){
                            closure( [&](){ this->unsubscribe(); },  std::forward<decltype(args)>(args)... );
                        }
                        , tmp_observable_values
                    );
                }
            public:

                // non-blocking
                template<class IntegralConstant, class Arg>
                void run(IntegralConstant, Arg &&arg) {
                    // update values cache
                    lock.lock();
                        auto &value = std::get<IntegralConstant::value>(observable_values);
                        value = std::forward<Arg>(arg);
						const auto tmp_observable_values = observable_values;
                    lock.unlock();

                    apply_closure(closure, tmp_observable_values, std::integral_constant<bool, add_unsubscibe_self>{});
                }

                void execute() {
                    execute(closure);
                }

                template<class ClosureT>
                void execute(ClosureT&& closure){
                    lock.lock();
                        const auto tmp_observable_values = observable_values;
                    lock.unlock();

                    apply_closure(std::forward<ClosureT>(closure), tmp_observable_values, std::integral_constant<bool, add_unsubscibe_self>{});
                }

            };


            template<class blocking_mode = reactive::default_blocking, bool add_unsubscibe_self = false, class Closure, class ...Observables>
            auto observe_impl(Closure &&closure, const std::shared_ptr<Observables>&... observables) {
                assert(and_all(observables...) && "all observables must exists on observe()!");

				// mix_threadsafe only safe to work in nonblocking mode
				constexpr const bool all_non_threadsafe	 = and_all(!Observables::threadsafe...);
				constexpr const bool some_non_threadsafe = or_all (!Observables::threadsafe...);
				constexpr const bool mix_threadsafe = some_non_threadsafe && !all_non_threadsafe;

				constexpr const bool efficiently_copyable = reactive::details::default_blocking::is_efficiently_copyable<typename Observables::Value...>::value;

				static_assert(!(mix_threadsafe && std::is_same<blocking_mode, reactive::blocking>::value)
					, "You trying to mix thread-safe with non-thread-safe properties in observer in blocking mode. Use default_blocking or nonblocking observer mode.");


				// If someone in blocking mode - play safe, work in blocking mode too
				constexpr const bool have_property_in_blocking_mode = or_all( std::is_same<typename Observables::blocking_mode, reactive::blocking>::value... );


				constexpr const bool is_default_blocking = std::is_same<blocking_mode, reactive::default_blocking>::value;

				constexpr const bool do_blocking = 
					( is_default_blocking
						? (mix_threadsafe ? false /* required */ : 
							(all_non_threadsafe && efficiently_copyable /* nonthread properties always in blocking mode */ ? false : have_property_in_blocking_mode)
						   )
						: std::is_same<blocking_mode, blocking>::value
					);

                using Observer = std::conditional_t<
                        do_blocking
                        , ObserverBlocking<add_unsubscibe_self, Closure, Observables...>
                        , ObserverNonBlocking<add_unsubscibe_self, Closure, Observables...>
                >;

                std::shared_ptr<Observer> observer = std::make_shared<Observer>(
                        std::forward<Closure>(closure), observables...
                );

                foreach([&](auto i, auto &observable) {
                    using I = decltype(i);
                    observable->subscribe(observer->tag, [observer](auto &&arg) {
                        observer->run(I{}, std::forward<decltype(arg)>(arg));
                    });
                }, observables...);

                return observer;
            }


            template<class blocking_mode = reactive::default_blocking, class Closure, class ...Observables>
            auto observe_w_unsubscribe_impl(Closure &&closure, const std::shared_ptr<Observables>&... observables) {
                return observe_impl<blocking_mode, true>(std::forward<Closure>(closure), observables...);
            };


        }   // namespace multiobserver
    }   // namespace details


    template<class blocking_mode = default_blocking, class Closure, class ...Observables>
    auto observe(Closure&& closure, Observables&... observables){
        return [
            observer = details::MultiObserver::observe_impl<blocking_mode>( std::forward<Closure>(closure), observables.shared_ptr()... )
        ](){
            observer->unsubscribe();
        };
    }


    template<class blocking_mode = default_blocking, class Closure, class ...Observables>
    auto observe_w_unsubscribe(Closure&& closure, Observables&... observables){
        return
        [
            observer = details::MultiObserver::observe_impl<blocking_mode, true>( std::forward<Closure>(closure), observables.shared_ptr()... )
        ](){
            observer->unsubscribe();
        };
    }


}

#endif //MULTIOBSERVER_H
