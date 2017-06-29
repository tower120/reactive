#pragma once


#include <type_traits>

namespace reactive {
	struct blocking {};
	struct nonblocking {};
	struct nonblocking_atomic {};
	struct default_blocking{};

	namespace details{
		namespace default_blocking{
			template<class Bool0>
			constexpr bool static and_all(Bool0 bool0){
				return bool0;
			}
			template<class Bool0, class ...Bools>
			constexpr bool static and_all(Bool0 bool0, Bools... bools){
				return bool0 && and_all(bools...);
			}


			template<class Int0>
			constexpr auto static and_plus(Int0 int0){
				return int0;
			}
			template<class Int0, class ...Ints>
			constexpr auto static and_plus(Int0 int0, Ints... ints){
				return int0 + and_plus(ints...);
			}

			// need this just to fix VS2017 v_141 compiler bug
			// #https://developercommunity.visualstudio.com/content/problem/69543/vs2017-v-141-parameter-pack-expansion-compilation.html
			template<class ...Ts>
			struct is_efficiently_copyable {
				static constexpr const bool value =
					and_all(std::is_copy_constructible<Ts>::value...) && and_plus(sizeof(Ts)...) <= 128;
			};
			template<class ...Ts>
			struct is_atomic {
				#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ < 7))
					// non standart compatible behavior. Non default constructible object can not be loaded from std::atomic
					// #https://developercommunity.visualstudio.com/content/problem/69560/stdatomic-load-does-not-work-with-non-default-cons.html
					static constexpr const bool value = 
						and_all(std::is_trivially_copyable<Ts>::value...) && and_all(std::is_nothrow_default_constructible<Ts>::value...);
				#else
					static constexpr const bool value = and_all(std::is_trivially_copyable<Ts>::value...);
				#endif
			};
		}
	}


	/**
	 * Non-blocking by default if
	 * All copy constructable & summary size <= 128
	 */
	template<class ...Ts>
	using get_default_blocking = std::conditional_t<
			details::default_blocking::is_efficiently_copyable<Ts...>::value
			, std::conditional_t<
				details::default_blocking::is_atomic<Ts...>::value
				, nonblocking_atomic
				, nonblocking
			>		
			, blocking
	>;

	template<class mode, class ...Ts>
	using get_blocking_mode = std::conditional_t<
	        std::is_same<mode, default_blocking>::value
			, get_default_blocking<Ts...>
			, mode
	>;
}
