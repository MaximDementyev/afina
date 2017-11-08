#include <afina/allocator/Simple.h>
#include <afina/allocator/Pointer.h>
#include <afina/allocator/Error.h>

namespace Afina {
namespace Allocator {

Simple::Simple(void *base, size_t size) : _base(base), _base_len(size) {
	//input sys_mem for te end our memory
	this->_first_free_block = static_cast<sys_block*>(base);
	this->_last_block = static_cast<sys_block*>(base);
	this->_size_descriptors = 0;
	this->_first_free_descriptor = nullptr;
	
	//create first free block
	const auto cur = static_cast<sys_block*>(base);
	cur->descriptor = nullptr;
	cur->size = size - this->sys_mem;
	cur->next_free = nullptr;
	cur->prev_free = nullptr;	
}


/**
 * TODO: semantics
 * @param size size_t
 */
Pointer Simple::alloc(size_t size) {
	if (size < this->min_mem) size = this->min_mem;//we can't give less
	system_memory_check(); //check if there is free space and place for the descriptor
	const auto cur = find_free_big_block(size);//find free and big enough block
	if (cur == nullptr) throw AllocError(AllocErrorType::NoMemory, "Try defraq"); //none of the blocks came up to size
	
	division_block(cur, size);//select the block of the desired size
	const auto desc = find_free_descriptor();//find free descrioptor

	//update descriptor
	cur->descriptor = desc;
	desc->block = cur;
	return Pointer(desc);
}

/**
 * TODO: semantics
 * @param ptr Pointer
 * @param size size_t
 */
void Simple::realloc(Pointer &ptr, size_t size) {
	const auto desc = ptr.get_descriptor();//get index from descriptor
	if (desc != nullptr) {
		const auto cur = desc->block;//find our block
		if (cur->size != size)
			if (cur->size < size) //check do we have to do something
				increase_block(cur, size, ptr);//increase size of block
			else reduce_block(cur, size);//reduce size of block
	} else ptr = alloc(size); // ptr is empty. alloc memory
}


/**
 * TODO: semantics
 * @param ptr Pointer
 */
void Simple::free(Pointer &ptr) {
	auto desc = ptr.get_descriptor();//get index from descriptor
	const auto cur = desc->block;//find our block
	combination_blocks(cur);//combination our block
	del_descriptor(desc); //del our descriptor
}

/**
 * TODO: semantics
 */
void Simple::defrag() {
	auto free_pos = reinterpret_cast<sys_block*>(this->_base);
	while (free_pos != this->_last_block && free_pos->descriptor != nullptr) // find first free block
		free_pos = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(free_pos) + this->sys_mem + free_pos->size);
	if (free_pos != this->_first_free_block) {//check whether it is necessary defrag
		auto cur = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(free_pos) + this->sys_mem + free_pos->size);
		while (true) {
			const auto next = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(cur) + this->sys_mem + cur->size);//find next block
			if (cur->descriptor != nullptr) {//block busy - move
				memcpy(free_pos, cur, this->sys_mem + cur->size);
				(free_pos->descriptor)->block = free_pos;//update descriptor
				free_pos = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(free_pos) + this->sys_mem + free_pos->size);//find new free place
			}
			if (cur == this->_last_block) break;
			cur = next;
		}
		//updae sysinfo
		this->_first_free_block = free_pos;
		this->_last_block = free_pos;
		//update info free block
		free_pos->descriptor = nullptr;
		free_pos->next_free = free_pos->prev_free = nullptr;
		free_pos->size = (reinterpret_cast<char*>(this->_base) + this->_base_len) - reinterpret_cast<char*>(free_pos) - sizeof(descriptors) * this->_size_descriptors - this->sys_mem;
	}
}


/**
 * TODO: semantics
 */
std::string Simple::dump() { return ""; }

} // namespace Allocator
} // namespace Afina
