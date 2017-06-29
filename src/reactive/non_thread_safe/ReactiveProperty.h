#pragma once

#include "../ReactiveProperty.h"

namespace reactive {
namespace non_thread_safe {

	template<class T>
	using ReactiveProperty = reactive::ReactiveProperty<T, reactive::blocking, false>;

}
}