#pragma once

#include "../details/Event.h"

namespace reactive {
namespace non_thread_safe {

	template<class ...Args>
	using Event = details::ConfigurableEvent<threading::dummy_mutex, threading::dummy_mutex, Args...>;

}
}