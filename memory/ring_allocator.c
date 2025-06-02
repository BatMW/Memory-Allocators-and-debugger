#include "stdbool.h"
#include "stdlib.h"
#include "ring_allocator.h"
#include "stdint.h"
#include "assert.h"

#if defined(_WIN32)
  #include <malloc.h>
  #define aligned_alloc(size, alignment) _aligned_malloc(size, alignment)
  #define aligned_free(ptr) _aligned_free(ptr)
#else
  #include <stdlib.h>
  #define aligned_free(ptr) free(ptr)
#endif
/*
 * Allocates a structure of block_size*nr_blocks bytes to the base pointer.
 * All pointers are NULL if allocation fails.
*/
MEM_Ring_Block_Allocator MEM_ring_block_allocator_init(size_t block_size, size_t nr_blocks) {
  MEM_Ring_Block_Allocator allocator = {0};

  // Preconditions: power-of-two block size, alignment-safe, and overflow-safe
  if ((block_size & (block_size - 1)) != 0) return allocator;
  if (block_size % sizeof(void*) != 0) return allocator;
  if (nr_blocks == 0 || block_size > SIZE_MAX / nr_blocks) return allocator;

  allocator.base = (char*)aligned_alloc(block_size, block_size*nr_blocks);
  if(allocator.base == NULL){
    return allocator;
  }
  allocator.block_size = block_size;
  allocator.nr_blocks = nr_blocks;

  allocator.head = 0;
  allocator.tail = 0;

  allocator.full = false;

  return allocator;
}

/*
 * Returns a pointer for the next block in the ring.
 * Returns NULL if head has caught up to tail.
*/
void* MEM_ring_block_allocator_alloc(MEM_Ring_Block_Allocator* allocator){
  if(allocator == NULL || allocator->base == NULL || allocator->full == true){
    return NULL;
  }
  
  char* ret = allocator->base + (allocator->head * allocator->block_size);
  allocator->head = (allocator->head + 1) & (allocator->nr_blocks - 1);
  allocator->full = (allocator->head == allocator->tail);
  return (void*)ret;

}

/*
 * Marks the block free and moves 'tail' appropriately
*/
void MEM_ring_block_allocator_free(MEM_Ring_Block_Allocator* allocator, void* block_ptr){
  if (!allocator || !allocator->base || !block_ptr) return;

  char* expected = allocator->base + (allocator->tail * allocator->block_size);
  if ((char*)block_ptr != expected) {
    assert(false && "Ring allocator: free order violated");
    return;
  }

  allocator->tail = (allocator->tail + 1) & (allocator->nr_blocks - 1);
  allocator->full = false;
}

/*
 * Deallocates 'base' and sets all pointers to NULL.
*/
void MEM_ring_block_allocator_destroy(MEM_Ring_Block_Allocator* allocator){
  if(allocator == NULL){
    return;
  }else if (allocator->base == NULL){
    return;
  }
  aligned_free(allocator->base);
  allocator->base = NULL;
  allocator->tail = 0;
  allocator->head = 0;
  allocator->block_size = 0;
  allocator->nr_blocks = 0;
}

