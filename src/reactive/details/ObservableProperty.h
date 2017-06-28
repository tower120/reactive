#ifndef REACTIVE_DETAILS_OBSERVABLEPROPERTY_H
#define REACTIVE_DETAILS_OBSERVABLEPROPERTY_H

#include "threading/upgrade_mutex.h"
#include "Event.h"

#include "ObservablePropertyFromThisBase.h"
#include "../blocking.h"

namespace reactive {
	// forward declaration 
	template<class, class>
	class ReactiveProperty;
	template<class, class>
	class ObservableProperty;

namespace details {

	// equality from #https://stackoverflow.com/a/36360646
	namespace details
	{
		template <typename T, typename R, typename = R>
		struct equality : std::false_type {};

		template <typename T, typename R>
		struct equality<T,R,decltype(std::declval<T>()==std::declval<T>())>
				: std::true_type {};
	}
	template<typename T, typename R = bool>
	struct has_equal_op : details::equality<T, R> {};

	namespace details {
		// ObservableProperty Does not inherit Settings, just because of non working VS 2017 "Empty base optimization"
		// #https://stackoverflow.com/questions/12701469/why-empty-base-class-optimization-is-not-working
		// #https://developercommunity.visualstudio.com/content/problem/69605/vs2017-c-empty-base-optimization-does-not-work-wit.html
		template<class T, class blocking_class>
		struct Settings {
			static constexpr const bool is_ObservablePropertyFromThis = std::is_base_of<ObservablePropertyFromThisBase, T>::value;

			using blocking_mode = get_blocking_mode<
				std::conditional_t<is_ObservablePropertyFromThis, blocking, blocking_class>
				, T>;
			/*using blocking_mode = get_blocking_mode<
				 blocking_class
				, T>;*/

			static const constexpr bool do_blocking  = std::is_same<blocking_mode, blocking>::value;
			static const constexpr bool atomic_value = std::is_same<blocking_mode, nonblocking_atomic>::value;

			using Lock = std::conditional_t<do_blocking
				, acme::upgrade_mutex
				, std::conditional_t<atomic_value, threading::dummy_mutex, threading::SpinLock<threading::SpinLockMode::Yield>>
			>;
		};

		// zero size dummy_mutex optimisation
		template<class Lock, class dummy = void>
		class ObservablePropertyLock;
		template<>
		class ObservablePropertyLock<threading::dummy_mutex, void> {
		protected:
			static threading::dummy_mutex m_lock;
		};
		threading::dummy_mutex ObservablePropertyLock<threading::dummy_mutex, void>::m_lock = threading::dummy_mutex{};

		template<class Lock>
		class ObservablePropertyLock<Lock, void> {
		protected:
			mutable Lock m_lock;
		};
	}


	// use with shared_ptr
	// thread-safe
	// reactive::nonblocking =  value will be copied twice on set
	//                          safe to change value right from the observer (may cause infinite loop)
	template<class T, class blocking_class = reactive::default_blocking>
	class ObservableProperty 
		: public details::ObservablePropertyLock< typename details::Settings<T, blocking_class>::Lock >
	{
		friend reactive::ReactiveProperty<T, blocking_class>;
		friend reactive::ObservableProperty<T, blocking_class>;
			
		using Self = ObservableProperty<T, blocking_class>;


		using Settings = details::Settings<T, blocking_class>;
		using BaseLock = details::ObservablePropertyLock< typename Settings::Lock >;

	public:
		using blocking_mode = typename Settings::blocking_mode;
	protected:
		static constexpr const bool is_ObservablePropertyFromThis = Settings::is_ObservablePropertyFromThis;
		static const constexpr bool do_blocking  = Settings::do_blocking;
		static const constexpr bool atomic_value = Settings::atomic_value;
		using Lock = typename Settings::Lock;

		using BaseLock::m_lock;

	public:
		using Value = T;

	protected:
		// variables order matters (for smaller object size)
		std::conditional_t<atomic_value, std::atomic<T>, T> value;
		mutable Event<const T&> event;

	private:

		void do_init_ObservablePropertyFromThis(std::true_type is_atomic) {
			T v = value;
			set_ObservablePropertyFromThis_ptr(v, this);
			value = v;
		}
		void do_init_ObservablePropertyFromThis(std::false_type is_atomic) {
			set_ObservablePropertyFromThis_ptr(value, this);
		}
		void init_ObservablePropertyFromThis(std::true_type is_ObservablePropertyFromThis) {
			do_init_ObservablePropertyFromThis(std::integral_constant<bool, atomic_value >{});
		}
		void init_ObservablePropertyFromThis(std::false_type is_ObservablePropertyFromThis) {}
		void init_ObservablePropertyFromThis() {
			init_ObservablePropertyFromThis(std::integral_constant<bool, is_ObservablePropertyFromThis>{});
		}

	public:
		ObservableProperty() {
			init_ObservablePropertyFromThis();
		}

		// in place construction
		template<
			class Arg, class ...Args
			, class = typename std::enable_if_t< !atomic_value && !std::is_same< std::decay_t<Arg>, Self >::value >
		>
		ObservableProperty(Arg&& arg, Args&&...args)
			: value( std::forward<Arg>(arg), std::forward<Args>(args)... ) {
			init_ObservablePropertyFromThis();
		};

		// for atomic
		template<
			class Arg, class ...Args
			, class = typename std::enable_if_t< atomic_value && !std::is_same< std::decay_t<Arg>, Self >::value >
			, typename = void
		>
		ObservableProperty(Arg&& arg, Args&&...args)
			: value( T(std::forward<Arg>(arg), std::forward<Args>(args)...) ) {
			init_ObservablePropertyFromThis();
		};


		// do not copy event list
		ObservableProperty(const ObservableProperty& other)
			:value(other.getCopy()) {
			init_ObservablePropertyFromThis();
		}
		ObservableProperty& operator=(const ObservableProperty& other) {
			set_value(other.getCopy());
			init_ObservablePropertyFromThis();
			return *this;
		}

		// move all (event list too)
		// non thread safe
		// will not be called
		ObservableProperty(ObservableProperty&&) = default;
		ObservableProperty& operator=(ObservableProperty&&) = default;

		// event control (non-blocking)
		template<class DelegateT>
		void operator+=(DelegateT&& closure) const {
			event += std::forward<DelegateT>(closure);
		}
		template<class Delegate>
		void subscribe(Delegate&& closure) const{
			event.subscribe(std::forward<Delegate>(closure));
		}
		template<class Fn>
		void subscribe(const DelegateTag& tag, Fn&& fn) const{
			event.subscribe(tag, std::forward<Fn>(fn));
		}
		template<class Delegate>
		void operator-=(Delegate&& closure) const {
			event -= std::forward<Delegate>(closure);
		}

	private:
		static bool need_trigger_event(const T& old_value, const T& new_value, std::true_type) {
			return !(new_value == old_value);
		}
		static bool need_trigger_event(const T& old_value, const T& new_value, std::false_type) {
			return true;
		}
	protected:
		static bool need_trigger_event(const T& old_value, const T& new_value) {
			return need_trigger_event(old_value, new_value, std::integral_constant<bool, has_equal_op<T>::value >{});
		}

		template<class Any>
		void set_value(Any&& any) {
			std::unique_lock<Lock> l(m_lock);
			set_value(any, std::move(l));
		}

	private:
		template<class Any, class AnyLock>
		void set_value_impl(Any&& any, std::unique_lock<AnyLock>&& lock, std::true_type do_block) {
			const bool need_event = need_trigger_event(any, this->value);

			//std::unique_lock<Lock> l(lock);
				this->value = std::forward<Any>(any);

			if (!need_event) return;

			std::shared_lock<AnyLock> sl(acme::upgrade_lock<AnyLock>(std::move(lock)));
				event(this->value);
		}
		template<class Any, class AnyLock>
		void set_value_impl(Any&& any, std::unique_lock<AnyLock>&& lock, std::false_type do_block) {
			const bool need_event = need_trigger_event(any, this->value);

			//lock.lock();
			this->value = std::forward<Any>(any);
				const T temp_value = this->value;
			lock.unlock();

			if (!need_event) return;
				event(temp_value);
		}
	protected:
		template<class Any, class AnyLock>
		void set_value(Any&& any, std::unique_lock<AnyLock>&& lock) {
			set_value_impl(std::forward<Any>(any), std::move(lock), std::integral_constant<bool, do_blocking>{});
		}

	public:
		// value setters (may block)
		void operator=(const T& value) {
			set_value(value);
		}
		void operator=(T&& value) {
			set_value(std::move(value));
		}

		T getCopy() const {
			const auto shared = lock();
			return shared.get();
		}

		class ReadLockNonCopy {
			friend Self;
		protected:
			const Self& self;
			std::shared_lock<typename Self::Lock> lock;		// Lock = upgarde_mutex

			ReadLockNonCopy(const Self& self)
					: self(self)
					, lock(self.m_lock)
			{}
		public:
			ReadLockNonCopy(ReadLockNonCopy&&) = default;
			ReadLockNonCopy(const ReadLockNonCopy&) = delete;

			const T& get() const {
				return self.value;
			}
			operator const T&() const {
				return get();
			}
			/*const T& operator()() const {
				return get();
			}*/
			const T* operator->() const {
				return &get();
			}
			const T& operator*() const{
				return get();
			}

			void unlock() {
				lock.unlock();
			}
		};


		class ReadLockAtomic {
			friend Self;
		protected:
			const T value;

			const auto& getValue(const Self& self, std::true_type is_atomic) const
			{
				return self.value;
			}
			const T& getValue(const Self& self, std::false_type is_atomic) const
			{
				std::unique_lock<Lock> tmp_lock(self.m_lock);					// SpinLock
				return self.value;
			}

			ReadLockAtomic(const Self& self)
                :value(getValue(self, std::integral_constant<bool, atomic_value>{}))
			{}
		public:
			ReadLockAtomic(ReadLockAtomic&&) = default;
			ReadLockAtomic(const ReadLockAtomic&) = delete;

			const T& get() const {
				return value;
			}
			operator const T&() const {
				return get();
			}
			/*const T& operator()() const {
				return get();
			}*/
			const T* operator->() const {
				return &get();
			}
			const T& operator*() const{
				return get();
			}

			void unlock(){}
		};

		using ReadLock = std::conditional_t<do_blocking, ReadLockNonCopy, ReadLockAtomic>;


		class WriteLockBase{
		protected:
			bool m_silent = false;

		public:
			void silent(bool be_silent = true){
				m_silent = be_silent;
			}
		};

		class WriteLockNonCopy : public WriteLockBase{
			friend Self;
		protected:
			Self* self;
			using Lock = typename Self::Lock;		// Lock = upgarde_mutex
			std::unique_lock<Lock> lock;

			WriteLockNonCopy(Self& self)
					: self(&self)
					, lock(self.m_lock)
			{}
			WriteLockNonCopy(Self& self, std::unique_lock<Lock>&& lock)
					: self(&self)
					, lock(std::move(lock))
			{}
		public:
			WriteLockNonCopy(WriteLockNonCopy&& other)
				: self(other.self)
				, lock(std::move(other.lock))
			{
				other.self = nullptr;
			}

			WriteLockNonCopy(const WriteLockNonCopy&) = delete;


			T& get() {
				return self->value;
			}
			operator T&() {
				return get();
			}
			/*const T& operator()() const {
				return get();
			}*/
			T* operator->() {
				return &get();
			}
			T& operator*(){
				return get();
			}

			void unlock() {
				if (!self) return;
				finish();
				self = nullptr;
			}

		private:
			void finish(std::true_type do_block) {
				std::shared_lock<Lock> sl(acme::upgrade_lock<Lock>(std::move(lock)));
				self->event(self->value);
			}
			void finish(std::false_type do_block) {
				const T temp_value = self->value;
				lock.unlock();

				self->event(temp_value);
			}
			void finish() {
				if (this->m_silent) return;
				finish(std::integral_constant<bool, do_blocking>{});
			}
		public:
			~WriteLockNonCopy(){
				if (!self) return; // moved?
				finish();
			}
		};


		// lockless
		class WriteLockAtomic  : public WriteLockBase{
			friend Self;
		protected:
			Self* self;
			T value;

			WriteLockAtomic(Self& self)
				: self(&self)
				, value(self.value)
			{}

			template<class AnyLock>
			WriteLockAtomic(Self& self, std::unique_lock<AnyLock>&& lock)
				: self(&self)
				, value(self.value)
			{}
		public:
			WriteLockAtomic(WriteLockAtomic&& other)
				: self(other.self)
				, value(std::move(other.value))
			{
				other.self = nullptr;
			}

			WriteLockAtomic(const WriteLockAtomic&) = delete;


			T& get() {
				return value;
			}
			operator T&() {
				return get();
			}
			/*const T& operator()() const {
			return get();
			}*/
			T* operator->() {
				return &get();
			}
			T& operator*() {
				return get();
			}

			void unlock() {
				if (!self) return;
				finish();
				self = nullptr;
			}

		private:
			void finish() {
				self->value = value;

				if (this->m_silent) return;

				self->event(value);
			}
		public:
			~WriteLockAtomic() {
				if (!self) return; // moved?
				finish();
			}
		};

		using WriteLock = std::conditional_t<atomic_value, WriteLockAtomic, WriteLockNonCopy>;

	protected:
		template<class AnyLock>
		WriteLock write_lock(std::unique_lock<AnyLock>&& lock) {
			return {*this, std::move(lock)};
		}

	public:
		// TODO: rename to read / write
		ReadLock lock() const {
			return {*this};
		}
		WriteLock write_lock() {
			return {*this};
		}

	private:
		void do_pulse(std::true_type do_blocking) const {
			std::shared_lock<Lock> sl(m_lock);
			event(this->value);
		}
		void do_pulse(std::false_type do_blocking) const {
			event(getCopy());
		}
	public:
		void pulse() const{
			do_pulse(std::integral_constant<bool, do_blocking>{});
		}

	};

}
}

#endif	//REACTIVE_DETAILS_OBSERVABLEPROPERTY_H
