#pragma once

#include "../ObservableProperty.h"

namespace reactive {
namespace non_thread_safe {

	template<class T>
	using ObservableProperty = reactive::ObservableProperty<T, reactive::blocking, false>;

}
}