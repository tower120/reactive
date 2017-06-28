#pragma once

#include <atomic>

#include "details/ObservablePropertyFromThisBase.h"
#include "details/utils/optional.hpp"
#include "ObservableProperty.h"

namespace reactive {

	// always in blocking mode
	template<class Derived>
	class ObservablePropertyFromThis : public details::ObservablePropertyFromThisBase {
	private:
		using Property = details::ObservableProperty<Derived, blocking>;

	protected:
		// write_lock()

		auto* property_from_this() {
			using Property = details::ObservableProperty<Derived>;
			return static_cast<Property*>(property_ptr);
		}
		const auto* property_from_this() const {
			return static_cast<const Property*>(property_ptr);
		}

		void notifyChange() {
			property_from_this()->pulse();
		}
	};

}