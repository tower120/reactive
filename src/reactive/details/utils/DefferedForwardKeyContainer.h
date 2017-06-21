#pragma once

#include "optional.hpp"

#include "DefferedForwardContainer.h"

#include "../threading/SpinLock.h"

namespace utils {

	namespace details {
		template<class Key, class T>
		struct DefferedForwardKeyContainerElement {
			Key key;
			T value;

			DefferedForwardKeyContainerElement(Key&& key)
				: key(std::move(key))
			{}
			DefferedForwardKeyContainerElement(const Key& key)
				: key(key)
			{}

			template<class KeyT, class Arg, class ...Args>
			DefferedForwardKeyContainerElement(KeyT&& key, Arg&& arg, Args&&...args)
				: key(std::forward<KeyT>(key))
				, value(std::forward<Arg>(arg), std::forward<Args>(args)...)
			{}

			DefferedForwardKeyContainerElement(DefferedForwardKeyContainerElement<Key, nonstd::optional<T>>&& other)
				: key(std::move(other.key))
				, value(std::move(*other.value))
			{}

			template<class Other>
			bool operator==(const Other& other) const{
				return key == other.key;
			}
		};
	}

	template<
		class Key, 
		class T,
		class ActionListLock	= threading::SpinLock<threading::SpinLockMode::Yield> /*std::mutex*/,
		class ListMutationLock	= std::shared_mutex,
		bool unordered = true
	>
	class DefferedForwardKeyContainer : protected DefferedForwardContainer< 
			details::DefferedForwardKeyContainerElement<Key, T>
			, details::DefferedForwardKeyContainerElement<Key, nonstd::optional<T> >
			, ActionListLock
			, ListMutationLock
			, unordered
		> 
	{

		using Base = DefferedForwardContainer<
			details::DefferedForwardKeyContainerElement<Key, T>
			, details::DefferedForwardKeyContainerElement<Key, nonstd::optional<T> >
			, ActionListLock
			, ListMutationLock
			, unordered
		>;

		//using DefferedElement = details::DefferedForwardKeyContainerElement<Key, nonstd::optional<T> >;

	public:
		DefferedForwardKeyContainer() {}

		template<class KeyT, class ...Args>
		void emplace(KeyT&& key, Args&&...args) {
			Base::emplace(std::forward<KeyT>(key),  std::forward<Args>(args)... );
		}

		template<class KeyT>
		void remove(KeyT&& key) {
			Base::remove(std::forward<KeyT>(key));
		}

		using Base::foreach;

		template<class Closure>
		void foreach_value(Closure&& closure) {
			Base::foreach([&](auto&& key_value) {
				closure(key_value.value);
			});
		}
	};
}