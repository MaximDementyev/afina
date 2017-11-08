#include <afina/allocator/Pointer.h>

namespace Afina {
namespace Allocator {

	Pointer::Pointer() {
		this->ptr = nullptr;
	}

	Pointer::Pointer(descriptors* new_ptr) {
		this->ptr = new_ptr;
	}

	void* Pointer::get() const {
		if (this->ptr == nullptr || this->ptr->free == true)
			return nullptr;
		return static_cast<void*>(reinterpret_cast<char*>(this->ptr->block) + 2*sizeof(size_t));
	}

	descriptors* Pointer::get_descriptor() const {
		if (this->ptr == nullptr) {
			return nullptr;
		}
		return this->ptr;
	}

} // namespace Allocator
} // namespace Afina
