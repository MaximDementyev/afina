#include <afina/allocator/Simple.h>
#include <afina/allocator/Pointer.h>
#include <afina/allocator/Error.h>

namespace Afina {
namespace Allocator {

void Simple::combination_blocks(sys_block *cur) {
	cur->descriptor = nullptr;
	auto flag_comb = false;
	if (cur != this->_last_block) {
		//our block is not last
		const auto next = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(cur) + this->sys_mem + cur->size); //index for next block
		if (next->descriptor == nullptr) {//combination whith next block
			//next block is empty
			flag_comb = true;
			cur->size += sys_mem + next->size;//update size block

			//update list cur
			cur->next_free = next->next_free;
			cur->prev_free = next->prev_free;

			//update list next free
			if (cur->prev_free != nullptr)
				(cur->prev_free)->next_free = cur;

			//update list prev free
			if (cur->next_free != nullptr)
				(cur->next_free)->prev_free = cur;

			//update sys info
			if (next == this->_first_free_block)
				this->_first_free_block = cur;
			if (next == this->_last_block)
				this->_last_block = cur;
		}
	}

	if (static_cast<void*>(cur) != this->_base) {
		sys_block* prev;
		if (flag_comb == true) prev = cur->prev_free;
		else prev = find_prev_free_block(cur);
		if (prev != nullptr && reinterpret_cast<sys_block*>(reinterpret_cast<char*>(prev) + this->sys_mem + prev->size) == cur) {//combination whith prev block
			//prev block is empty
			prev->size += this->sys_mem + cur->size; //update size of block
			
			//update sys info
			if (cur == this->_last_block)
				this->_last_block = prev;
			
			//cur has link next and prev free
			if (flag_comb == true) del_block_from_list(cur);
			flag_comb = true;
		}
	}

	if (flag_comb == false)//Our block is surrounded by busy
		add_in_first_free(cur);//we inseart cur block it first place in list free blocks
}

void Simple::division_block(sys_block *cur, size_t size) {
	const auto next = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(cur) + this->sys_mem + size); //index for new block

	if (cur->size - size > this->sys_mem && cur->size - size - this->sys_mem >= this->min_mem) { //we can division this block
		//update size block
		next->size = cur->size - size - this->sys_mem;
		cur->size = size;

		next->descriptor = nullptr;//update state blocks

		//update list next_free
		next->next_free = cur->next_free;
		if (cur->prev_free != nullptr)
			(cur->prev_free)->next_free = next;

		//update list prev_free
		next->prev_free = cur->prev_free;
		if (cur->next_free != nullptr)
			(cur->next_free)->prev_free = next;

		//update sysinfo
		if (cur == this->_last_block)
			this->_last_block = next;
		if (cur == this->_first_free_block)
			this->_first_free_block = next;

	} else { //we can't division this block
		//update list next_free 
		if (cur->prev_free != nullptr)
			(cur->prev_free)->next_free = cur->next_free;
		//update list prev_free
		if (cur->next_free != nullptr)
			(cur->next_free)->prev_free = cur->prev_free;

		//update sysinfo
		if (cur == this->_first_free_block)
			this->_first_free_block = cur->next_free;
	}
}

void Simple::increase_block(sys_block* cur, size_t size, Pointer &ptr) {
	const auto next = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(cur) + this->sys_mem + cur->size); //index for next block
	if (cur != this->_last_block && next->descriptor == nullptr) { //next block is free. use it
		if (next->size - (size - cur->size) > this->min_mem) {//we can division next block
			block_boundary_change(cur, *next, next == this->_first_free_block, next == this->_last_block, size);
		} else { // we can't devesion next block
			// combination block and give to the user
			//update size
			cur->size += this->sys_mem + next->size;
			//update sys_info
			if (next == this->_last_block)
				this->_last_block = cur;

			del_block_from_list(next);//del next block from all list
		}
	} else { //next block is busy, we have to copy our block in new place
		system_memory_check();//check if there is free space and place for the descriptor
		//find new place
		const auto new_cur = find_free_big_block(size);
		if (new_cur == nullptr) throw AllocError(AllocErrorType::NoMemory, "Try defraq"); //none of the blocks came up to size
		division_block(new_cur, size);
		memcpy(new_cur, cur, this->sys_mem + cur->size); //copy our blockin new place
		(new_cur->descriptor)->block = new_cur; // update descriptor
		combination_blocks(cur);//delete block
	}
}

void Simple::reduce_block(sys_block* cur, size_t size) {
	const auto next = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(cur) + this->sys_mem + cur->size);
	if (cur != this->_last_block && next->descriptor == nullptr)//next block is empty, give the excess to the next block
		block_boundary_change(cur, *next, next == this->_first_free_block, next == this->_last_block, size);
	else { //we working only with memory cur_block
		if (cur->size - size > this->sys_mem && cur->size - size - this->sys_mem > this->min_mem) {//we can insert free block
			const auto new_next = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(cur) + this->sys_mem + size); //index for new block

			//update size block
			new_next->size = cur->size - size - this->sys_mem;
			cur->size = size;

			//update sysinfo
			if (cur == this->_last_block)
				this->_last_block = new_next;

			add_in_first_free(new_next);//we inseart cur block it first place in list free blocks
		} //else we do not do anything
	}
}

void Simple::block_boundary_change(sys_block* cur, sys_block next, bool is_first_free, bool is_last_block, size_t size) {
	const auto new_next = reinterpret_cast<sys_block*>(reinterpret_cast<char*>(cur) + this->sys_mem + size);//find new index next block
	//block is free
	new_next->descriptor = nullptr;

	//update size
	new_next->size = next.size - (size - cur->size);
	cur->size = size;

	//update sysinfo
	if (is_first_free == true)
		this->_first_free_block = new_next;
	if (is_last_block == true)
		this->_last_block = new_next;
	
	//copy block list from next to new_next
	new_next->prev_free = next.prev_free;
	new_next->next_free = next.next_free;

	//update list next_free
	if (new_next->prev_free != nullptr)
		(new_next->prev_free)->next_free = new_next;

	//update list prev_free
	if (new_next->next_free != nullptr)
		(new_next->next_free)->prev_free = new_next;
}

sys_block* Simple::find_free_big_block(size_t size) const {
	auto cur = this->_first_free_block;
	while (true) {//find free and big enough block
		if (cur->size >= size)
			if (cur == this->_last_block) {
				if (this->_first_free_descriptor != nullptr)
					return cur;
				if (cur->size - sizeof(descriptors) > size)
					return cur;
			} else return cur;
			if (cur->next_free == nullptr)
				break;
			cur = cur->next_free;
	}
	return nullptr;
}

descriptors* Simple::find_free_descriptor() {
	if (this->_first_free_descriptor != nullptr) {
		auto *desc = this->_first_free_descriptor;//find first free descriptor and take it out
		this->_first_free_descriptor = reinterpret_cast<descriptors*>(desc->block);//update first free descriptor
		desc->free = false;//our descriptor is not free now
		return desc;
	}
	//there is no free descriptor
	this->_last_block->size -= sizeof(descriptors);//reduce the size of the latter block
	const auto desc = reinterpret_cast<descriptors*>(reinterpret_cast<char*>(this->_last_block) + this->sys_mem + this->_last_block->size);//add free descriptor
	this->_size_descriptors += 1;//increase the number of descriptor
	desc->free = false;//our descriptor is not free now
	return desc;
}

sys_block* Simple::find_prev_free_block(sys_block* block) const {
	auto cur = this->_first_free_block;
	while (cur->next_free != nullptr && cur->next_free < block) {//find prev free block
		cur = cur->next_free;
	}
	return cur;
}

void Simple::del_block_from_list(sys_block* cur) {//del block from all list
	if (cur->prev_free != nullptr)
		(cur->prev_free)->next_free = cur->next_free;
	if (cur->next_free != nullptr)
		(cur->next_free)->prev_free = cur->prev_free;

	if (cur == this->_first_free_block)
		this->_first_free_block = cur->next_free;
}

void Simple::add_in_first_free(sys_block* cur) {//inseart cur block it first place in list free blocks
	cur->prev_free = nullptr;
	cur->next_free = this->_first_free_block;
	(cur->next_free)->prev_free = cur;
	this->_first_free_block = cur;
}

void Simple::system_memory_check() const {//check if there is free space and place for the descriptor
	if (this->_first_free_block == nullptr) throw AllocError(AllocErrorType::NoMemory, "Try defraq");//no free block
	if (this->_first_free_descriptor == nullptr) {//check memmory for new descriptor
		if (this->_last_block->descriptor != nullptr)
			throw AllocError(AllocErrorType::NoMemory, "Try defraq");//no memory for new descriptor
		if (this->_last_block->size - sizeof(descriptors) < this->min_mem)
			throw AllocError(AllocErrorType::NoMemory, "Try defraq");//no memory for new descriptor
	}
}

void Simple::del_descriptor(descriptors* desc) {
	desc->free = true;
	if (reinterpret_cast<char*>(desc) != reinterpret_cast<char*>(this->_base) + this->_base_len - (sizeof(descriptors) * this->_size_descriptors)) {//our descriptor is not last
		if (desc < this->_first_free_descriptor) {
			auto tmp_desc = desc;
			while (tmp_desc->free == false) //find nearest free descriptor
				tmp_desc = reinterpret_cast<descriptors*>(reinterpret_cast<char*>(tmp_desc) + sizeof(descriptors));
			//update list
			desc->block = tmp_desc->block;
			tmp_desc->block = reinterpret_cast<sys_block*>(desc);
		} else {
			//desc now first free descriptor - update list
			desc->block = reinterpret_cast<sys_block*>(this->_first_free_descriptor);
			this->_first_free_descriptor = desc;
		}
	} else {//our descriptor is last
		while (desc->free == true && desc != this->_first_free_descriptor) {//remove all free descriptors from the end
			desc->block = nullptr;
			this->_size_descriptors -= 1;
			this->_last_block->size += sizeof(descriptors);
			desc = reinterpret_cast<descriptors*>(reinterpret_cast<char*>(desc) + sizeof(descriptors));
		}
	}
}
} // namespace Allocator
} // namespace Afina