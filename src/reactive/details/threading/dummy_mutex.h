#pragma once

namespace threading {
	struct dummy_mutex {
		void lock() {}
		bool try_lock() { return true; };
		void unlock() {}

		void lock_shared() {}
		bool try_lock_shared() { return true; };
		void unlock_shared() {}

		/*void shared_lock() {}
		void shared_unlock() {}*/

		void unlock_and_lock_shared() {};
		void unlock_upgrade_and_lock() {};


		void lock_upgrade() {};
		bool try_lock_upgrade() { return true; };
		void unlock_upgrade() {};

		bool try_unlock_shared_and_lock_upgrade() { return true; };

		template<typename ...Args>
		bool try_unlock_shared_and_lock_upgrade_for(Args&&...) { return true; };
		template<typename ...Args>
		bool try_unlock_shared_and_lock_upgrade_until(Args&&...) { return true; };

		void unlock_and_lock_upgrade() {};
		void unlock_upgrade_and_lock_shared() {};
	};
}