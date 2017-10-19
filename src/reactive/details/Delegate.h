#pragma once

#include <vector>
#include <functional>
#include <deque>
#include <atomic>

namespace reactive {

	namespace details {
	namespace Delegate {
		// 0 - reserved for empty tag?
		static std::atomic<unsigned long long> delegate_uuid{ 1 };	// should be enough approx. for 1000 years at 3Ghz continuous incrementation
	}
	}

	// pass by copy
	class DelegateTag {
		unsigned long long tag;

	protected:
		DelegateTag(std::nullptr_t)
			:tag(0)
		{}
	public:
		DelegateTag()
			: tag(details::Delegate::delegate_uuid.fetch_add(1))
		{}

		bool operator==(const DelegateTag& other) const {
			return tag == other.tag;
		}
		bool operator!=(const DelegateTag& other) const {
			return tag != other.tag;
		}
	};

	namespace details {
		class DelegateTagEmpty : public DelegateTag {
			using DelegateTag::DelegateTag;
		public:
			DelegateTagEmpty() : DelegateTag(nullptr) {};
			using DelegateTag::operator==;
			using DelegateTag::operator!=;
		};

		class DelegateBase {};
	}


	template<class ...Args>
	class Delegate : details::DelegateBase {
		using Self = Delegate<Args...>;
		DelegateTag m_tag;

		using Fn = std::function<void(Args...)>;
		Fn m_fn;

	public:
		template<class FnT, 
			class = std::enable_if_t< !std::is_same< std::decay_t<FnT>, Self >::value>
		>
		Delegate(FnT&& fn)
			:m_fn(std::forward<FnT>(fn))
		{}

		template<class FnT>
		Delegate(DelegateTag tag, FnT&& fn)
			: m_tag(tag)
			, m_fn(std::forward<FnT>(fn))
		{}


		void operator=(const Fn& fn) {
			fn = fn;
		}
		void operator=(Fn&& fn) {
			fn = std::move(fn);
		}

		const DelegateTag& tag() const {
			return m_tag;
		}
		const Fn& function() const {
			return m_fn;
		}

	};

}
