#pragma once

#include <vector>
#include <algorithm>
#include <mutex>
#include <shared_mutex>

#include "../threading/SpinLock.h"
#include "../threading/dummy_mutex.h"

namespace utils {
	/*
		Implementation details:
		Possible to implement with one deque, but:
		It is much faster (2-10 times) to use 2 vectors, then one deque
	*/

	namespace details{
		// zero-size (static) locks for dummy_mutex
		template<class ActionListLock, class ListMutationLock>
		class DeferredForwardContainerBase {
		protected:
			ListMutationLock list_mutation_lock;
			ActionListLock action_list_lock;
		public:
			DeferredForwardContainerBase() {};
		};

		template<>
		class DeferredForwardContainerBase<threading::dummy_mutex, threading::dummy_mutex> {
		protected:
			inline static threading::dummy_mutex action_list_lock;
			inline static threading::dummy_mutex list_mutation_lock;
		public:
			DeferredForwardContainerBase() {};
		};
	}

	// thread safe	(use threading::dummy_mutex to disable)
	// safe to add/remove while iterate
	// non-blocking
	// unordered (by default, but may be ordered) - does not require copy on remove
	template< 
		class T, 
		class DeferredActionValue = T,
		class ActionListLock	= threading::SpinLock<threading::SpinLockMode::Adaptive> /*std::mutex*/,
		class ListMutationLock	= std::shared_mutex,
		bool unordered = true
	>
	class DeferredForwardContainer : public details::DeferredForwardContainerBase<ActionListLock, ListMutationLock> {
		using Base = details::DeferredForwardContainerBase<ActionListLock, ListMutationLock>;
		using Base::action_list_lock;
		using Base::list_mutation_lock;

		enum class Action { remove, add };
		struct DeferredActionElement {
			Action action;
			DeferredActionValue value;

			template<class ActionT, class Arg, class ...Args>
			DeferredActionElement(ActionT&& action, Arg&& arg, Args&&... args)
				: action(std::forward<ActionT>(action))
				, value(std::forward<Arg>(arg), std::forward<Args>(args)...)
			{}
		};
		std::vector<DeferredActionElement> deferredActionList;

		using ListElement = T;
		using List = std::vector<ListElement>;
		List list;

		void apply_actions() {
			std::unique_lock<ActionListLock> l(action_list_lock);
			if (deferredActionList.empty()) return;

			std::unique_lock<ListMutationLock> l2(list_mutation_lock);
			for (DeferredActionElement& action : deferredActionList) {
				if (action.action == Action::add) {
					list.emplace_back(std::move(action.value));
				} else {
					// remove one
					auto it = std::find_if(list.begin(), list.end(), [&](ListElement& element) {
						return (element == action.value);
					});
					
					if (unordered) {
						if (it != list.end()) {
							std::iter_swap(it, list.end() - 1);
							list.pop_back();
						}
					} else {
						if (it != list.end()) {
							list.erase(it);
						}
					}
				}
			}

			deferredActionList.clear();
		}

	public:
		DeferredForwardContainer() {}

		template<class ...Args>
		void emplace(Args&&...args) {
			std::unique_lock<ActionListLock> l(action_list_lock);
			deferredActionList.emplace_back(Action::add, std::forward<Args>(args)...);
		}

		template<class ...Args>
		void remove(Args&&...args) {
			std::unique_lock<ActionListLock> l(action_list_lock);
			deferredActionList.emplace_back(Action::remove, std::forward<Args>(args)...);
		}

		template<class Closure>
		void foreach(Closure&& closure) {
			apply_actions();

			std::shared_lock<ListMutationLock> l(list_mutation_lock);
			for (ListElement& element : list) {
				closure(element);
			}
		}
	};

}