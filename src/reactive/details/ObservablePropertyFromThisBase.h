#pragma once

namespace reactive {
namespace details {
	class ObservablePropertyFromThisBase {
		friend void set_ObservablePropertyFromThis_ptr(ObservablePropertyFromThisBase& self, void* ptr) {
			self.property_ptr = ptr;
		}
	protected:
		void* property_ptr;
	};
}
}
