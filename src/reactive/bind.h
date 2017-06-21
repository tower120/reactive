#ifndef REACTIVE_BIND_H
#define REACTIVE_BIND_H

#include <memory>
#include "observer.h"

namespace reactive {

	namespace details {
		template<class blocking_mode, class Obj, class Closure, class ...Observables>
		auto bind(const std::shared_ptr<Obj>& obj, Closure&& closure, const Observables&... observables) {
			auto observer = /*details::*/MultiObserver::observe_w_unsubscribe_impl<blocking_mode>(
				[
					closure = std::forward<Closure>(closure)
					, obj_weak = std::weak_ptr<Obj>(obj)
				](auto&& unsubscribe_self, auto&&...args){
				auto obj_ptr = obj_weak.lock();
				if (!obj_ptr) {
					unsubscribe_self();
					return;
				}

				closure(std::forward<decltype(unsubscribe_self)>(unsubscribe_self), std::move(obj_ptr), std::forward<decltype(args)>(args)...);
			}
			, observables.shared_ptr()...);

			observer->execute();

			return [observer]() { observer->unsubscribe(); };
		}
	}


	template<class blocking_mode = default_blocking, class Obj, class Closure, class ...Observables>
	auto bind(const std::shared_ptr<Obj>& obj, Closure&& closure, const Observables&... observables) {
		return details::bind<blocking_mode>(obj,
			[closure = std::forward<Closure>(closure)](auto&& unsubscribe_self, auto&&...args) {
				closure(std::forward<decltype(args)>(args)...);
			}
			, observables...
		);
	}
	
	template<class blocking_mode = default_blocking, class Obj, class Closure, class ...Observables>
	auto bind_w_unsubscribe(const std::shared_ptr<Obj>& obj, Closure&& closure, const Observables&... observables) {
		return details::bind<blocking_mode>(obj, std::forward<Closure>(closure), observables...);
	}

}

#endif //REACTIVE_BIND_H
