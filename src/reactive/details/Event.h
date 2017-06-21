#ifndef REACTIVE_DETAILS_EVENT_H
#define REACTIVE_DETAILS_EVENT_H

#include <vector>
#include <deque>

#include <functional>
#include <shared_mutex>


#include "Delegate.h"

#include "threading/SpinLock.h"
#include "utils/DefferedForwardKeyContainer.h"

namespace reactive {
namespace details {

	// thread-safe, non-blocking
	// subscribe/unsubscribe may be deffered to next () call
	template<
		class ActionListLock /*= threading::SpinLock<threading::SpinLockMode::Adpative>*/,
		class ListMutationLock /*= std::shared_mutex*/,
		class ...Args>
	class ConfigurableEventBase {
		using Delegate = typename reactive::Delegate<Args...>;
	protected:
		utils::DefferedForwardKeyContainer<
			DelegateTag, std::function<void(Args...)>
			, ActionListLock, ListMutationLock
		> list;

	public:
		template<class Fn>
		void subscribe(const DelegateTag& tag, Fn&& fn) {
			list.emplace(tag, std::forward<Fn>(fn));
		}

		template<class ...ArgsT>
		void subscribe(const typename reactive::Delegate<ArgsT...>& delegate) {
			list.emplace(delegate.tag(), delegate.function());
		}

		template<class Closure, class = std::enable_if_t< !std::is_base_of<reactive::details::DelegateBase, std::decay_t<Closure>>::value > >
		void operator+=(Closure&& closure) {
			subscribe(DelegateTagEmpty{}, std::forward<Closure>(closure));
		}
		template<class ...ArgsT>
		void operator+=(const typename reactive::Delegate<ArgsT...>& delegate) {
			subscribe(delegate);
		}


		void operator-=(const DelegateTag& tag) {
			list.remove(tag);
		}
		void operator-=(const Delegate& delegate) {
			list.remove(delegate.tag());
		}


		template<class ...Ts>
		void operator()(Ts&&...ts) {
			list.foreach_value([&](auto& delegate) {
				//delegate(std::forward<Ts>(ts)...);	// can't move here
				delegate(ts...);	// can't move here
			});
		}
	};


	/*
	template<
		class ActionListLock,
		class ListMutationLock,
		class ...Args>
	class ConfigurableEventWithAction 
		: public ConfigurableEventBase<ActionListLock, ListMutationLock, Args...>
	{
		using Base = ConfigurableEventBase<ActionListLock, ListMutationLock, Args...>;
		using Action = std::function<void()>;

		using Base::list;
	public:
		using Base::operator+=;
		using Base::operator-=;
		using Base::subscribe;

		void subscribe(Action& action) {
			list.emplace(&action, [action](auto&&...){ action(); });
		}
		void operator+=(Action& action) {
			subscribe(action);
		}
		void operator-=(const Action& action) {
			list.remove(static_cast<const void*>(&action));
		}
	};
	*/

	template<
		class ActionListLock /*= threading::SpinLock<threading::SpinLockMode::Adpative>*/,
		class ListMutationLock /*= std::shared_mutex*/,
		class ...Args>
	using ConfigurableEvent = ConfigurableEventBase<ActionListLock, ListMutationLock, Args...>
		/*std::conditional_t<
		sizeof...(Args) == 0
		, ConfigurableEventBase<ActionListLock, ListMutationLock, Args...>
		, ConfigurableEventWithAction<ActionListLock, ListMutationLock, Args...>
	>*/;


	template<class ...Args>
	using Event = ConfigurableEvent<
		threading::SpinLock<threading::SpinLockMode::Yield>, 
		std::shared_mutex, 
		Args...
	>;
}
}

#endif	//REACTIVE_DETAILS_EVENT_H