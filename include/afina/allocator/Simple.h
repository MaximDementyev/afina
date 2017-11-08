#ifndef AFINA_ALLOCATOR_SIMPLE_H
#define AFINA_ALLOCATOR_SIMPLE_H

#include <string>
#include <cstddef>
#include <memory.h>
#include <afina/allocator/Pointer.h>

namespace Afina {
namespace Allocator {
	struct descriptors;
	struct sys_block {
		descriptors *descriptor;
		size_t size;
		sys_block *prev_free;
		sys_block *next_free;
	};
	#pragma pack(push,1)
	struct descriptors {
		sys_block* block;
		bool free;
	};
	#pragma pack(pop)
// Forward declaration. Do not include real class definition
// to avoid expensive macros calculations and increase compile speed
class Pointer;



/**
 * Wraps given memory area and provides defagmentation allocator interface on
 * the top of it.
 *
 * Allocator instance doesn't take ownership of wrapped memmory and do not delete it
 * on destruction. So caller must take care of resource cleaup after allocator stop
 * being needs
 */
// TODO: Implements interface to allow usage as C++ allocators
class Simple {
public:
	
    Simple(void *base, const size_t size);

    /**
     * TODO: semantics
     * @param N size_t
     */
    Pointer alloc(size_t N);

    /**
     * TODO: semantics
     * @param p Pointer
     * @param N size_t
     */
    void realloc(Pointer &p, size_t N);
	

	/**
     * TODO: semantics
     * @param p Pointer
     */
    void free(Pointer &p);

    /**
     * TODO: semantics
     */
    void defrag();
	

	/**
     * TODO: semantics
     */
    std::string dump();

private:
	void* _base;
    const size_t _base_len;
	sys_block* _first_free_block;
	sys_block* _last_block;
	size_t _size_descriptors;
	descriptors* _first_free_descriptor;
	
	size_t sys_mem = 2 * sizeof(size_t);
	size_t min_mem = 2 * sizeof(size_t);
	//*//
	sys_block* find_prev_free_block(sys_block*) const;
	void del_block_from_list(sys_block* cur);
	void add_in_first_free(sys_block* cur);
	void system_memory_check() const;
	void del_descriptor(descriptors* desc);
	void increase_block(sys_block* cur, size_t size, Pointer &ptr);
	void reduce_block(sys_block* cur, size_t size);
	void block_boundary_change(sys_block* cur, sys_block next, bool is_first_free, bool is_last_block, size_t size);
	void combination_blocks(sys_block*);
	void division_block(sys_block*, size_t size);
	sys_block* find_free_big_block(size_t size) const;
	descriptors* find_free_descriptor();
};

} // namespace Allocator
} // namespace Afina
#endif // AFINA_ALLOCATOR_SIMPLE_H
