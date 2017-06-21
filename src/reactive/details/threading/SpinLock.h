#pragma once

#include <atomic>
#include <thread>

namespace threading {

	enum class SpinLockMode { Nonstop, Yield, Sleep, Adpative };

	template<SpinLockMode mode = SpinLockMode::Adpative>
	class SpinLock {
		std::atomic_flag spinLock = ATOMIC_FLAG_INIT;

		// not in use
		void adaptive_lock_v1() {
			if (!spinLock.test_and_set(std::memory_order_acquire)) {
				// fast return
				return;
			}

			for (int i = 0;i < 100;i++) {
				if (!spinLock.test_and_set(std::memory_order_acquire)) {
					return;
				}
				std::this_thread::yield();
			}

			while (true) {
				if (!spinLock.test_and_set(std::memory_order_acquire)) {
					return;
				}

				using namespace std::chrono;
				std::this_thread::sleep_for(1ns);
			}
		}

		void adaptive_lock() {
			if (!spinLock.test_and_set(std::memory_order_acquire)) {
				// fast return
				return;
			}

			while (true) {
				for (int i = 0; i < 100; i++) {
					if (!spinLock.test_and_set(std::memory_order_acquire)) {
						return;
					}
					std::this_thread::yield();
				}

				using namespace std::chrono;
				std::this_thread::sleep_for(1ns);
			}
		}

	public:
		SpinLock(){}
		SpinLock(const SpinLock&) = delete;
		SpinLock(SpinLock&&) = delete;

		void lock() {
			if (mode == SpinLockMode::Adpative) {
				adaptive_lock();
				return;
			}

			while (true) {
				if (!spinLock.test_and_set(std::memory_order_acquire)) {
					return;
				}

				if (mode == SpinLockMode::Sleep) {
					using namespace std::chrono;
					std::this_thread::sleep_for(1ns);
				}
				else if (mode == SpinLockMode::Yield) {
					std::this_thread::yield();
				}
			}
		}

		void unlock() {
			spinLock.clear(std::memory_order_release);               // release lock
		}
	};
}