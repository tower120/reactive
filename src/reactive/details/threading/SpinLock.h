#pragma once

#include <atomic>
#include <thread>

#include "details/SpinLockSpinner.h"

namespace threading {

	template<SpinLockMode mode = SpinLockMode::Adaptive>
	class SpinLock {
		std::atomic_flag spinLock = ATOMIC_FLAG_INIT;

	public:
		SpinLock(){}
		SpinLock(const SpinLock&) = delete;
		SpinLock(SpinLock&&) = delete;

		bool try_lock(){
			return !spinLock.test_and_set(std::memory_order_acquire);
		}

		void lock() {
            details::SpinLockSpinner::spinWhile([&](){
                return spinLock.test_and_set(std::memory_order_acquire);
            });
		}

		void unlock() {
			spinLock.clear(std::memory_order_release);               // release lock
		}
	};
}