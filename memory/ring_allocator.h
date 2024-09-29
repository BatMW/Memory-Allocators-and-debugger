#ifndef MEM_RING_ALLOCATOR
#define MEM_RING_ALLOCATOR
#include "stddef.h"

typedef struct MEM_Ring_Block_Allocator{
  void* base;
  void* head;
  void* tail;
  size_t block_size;
  size_t nr_blocks;
} MEM_Ring_Block_Allocator;

/*
 * Allocates a structure of (block_size+1)*nr_blocks bytes to the base pointer.
 * All pointers are NULL if allocation fails.
*/
MEM_Ring_Block_Allocator MEM_ring_block_allocator_init(size_t block_size, size_t nr_blocks);

/*
 * Returns a pointer for the next block in the ring.
 * Returns NULL if head has caught up to tail.
*/
void* MEM_ring_block_allocator_alloc(MEM_Ring_Block_Allocator* allocator);

/*
 * Marks the block free and moves 'tail' appropriately
 * May or may not allow a new allocation.
*/
void MEM_ring_block_allocator_free(MEM_Ring_Block_Allocator* allocator, void* block_ptr);

/*
 * Deallocates base and sets all pointers to NULL.
*/
void MEM_ring_block_allocator_destroy(MEM_Ring_Block_Allocator* allocator);

#endif
