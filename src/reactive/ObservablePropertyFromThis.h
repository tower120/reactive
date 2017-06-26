#pragma once

#include <atomic>

#include "details/ObservablePropertyFromThisBase.h"
#include "details/utils/optional.hpp"
#include "ObservableProperty.h"

namespace reactive {

	template<class Derived>
	class ObservablePropertyFromThis : public details::ObservablePropertyFromThisBase {
		using Property = details::ObservableProperty<Derived>;

	protected:
		Property* property_from_this() {
			return static_cast<Property*>(property_ptr);
		}
		const Property* property_from_this() const {
			return static_cast<const Property*>(property_ptr);
		}

		void notifyChange() {
			property_from_this()->pulse();
		}
	};

}