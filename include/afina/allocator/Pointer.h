#ifndef AFINA_ALLOCATOR_POINTER_H
#define AFINA_ALLOCATOR_POINTER_H
#include <afina/allocator/Simple.h>

namespace Afina {
namespace Allocator {

	// Forward declaration. Do not include real class definition
// to avoid expensive macros calculations and increase compile speed
class Simple;
struct descriptors;
struct sys_block;

class Pointer {
public:
	Pointer();
	Pointer(descriptors* new_ptr);
	void* get() const;
	descriptors *get_descriptor() const;
private:
	descriptors* ptr;
};

} // namespace Allocator
} // namespace Afina

#endif // AFINA_ALLOCATOR_POINTER_H
