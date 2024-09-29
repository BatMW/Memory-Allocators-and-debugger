#include "stdbool.h"
#include "stdlib.h"
#include "ring_allocator.h"

inline static void* ring_block_next_head(MEM_Ring_Block_Allocator* allocator){
  char *next = (char*)allocator->head + allocator->block_size;
  if(next >= (char*)allocator->base + (allocator->block_size*allocator->nr_blocks)){
    next = allocator->base;
  }
  return (void*)next;
}
inline static void* ring_block_next_tail(MEM_Ring_Block_Allocator* allocator){
  char *next = (char*)allocator->tail + allocator->block_size;
  if(next >= (char*)allocator->base + (allocator->block_size*allocator->nr_blocks)){
    next = allocator->base;
  }
  return (void*)next;
}

/*
 * Allocates a structure of (block_size+1)*nr_blocks bytes to the base pointer.
 * All pointers are NULL if allocation fails.
*/
 MEM_Ring_Block_Allocator MEM_ring_block_allocator_init(size_t block_size, size_t nr_blocks){
  MEM_Ring_Block_Allocator allocator;
  allocator.base = calloc(block_size+sizeof(bool), nr_blocks);
  allocator.head = allocator.base;
  allocator.tail = allocator.base;
  if(allocator.base != NULL){
    allocator.block_size = block_size+sizeof(bool);
    allocator.nr_blocks = nr_blocks;
  }else{
    allocator.block_size = 0;
    allocator.nr_blocks = 0;
  }
  return allocator;
}

/*
 * Returns a pointer for the next block in the ring.
 * Returns NULL if head has caught up to tail.
*/
void* MEM_ring_block_allocator_alloc(MEM_Ring_Block_Allocator* allocator){
  if(allocator == NULL){
    return NULL;
  }else if(allocator->base == NULL){
    return NULL;
  }
  
  if(allocator->head == allocator->tail && *(bool*)allocator->head == true){//Full
    return NULL;
  }else{
    void* old_head = allocator->head;
    *(bool*)old_head = true;
    allocator->head = ring_block_next_head(allocator);
    return (void*)((char*)old_head+sizeof(bool)); 
  }

}

/*
 * Marks the block free and moves 'tail' appropriately
 * May or may not allow a new allocation.
*/
void MEM_ring_block_allocator_free(MEM_Ring_Block_Allocator* allocator, void* block_ptr){
  if(allocator == NULL){
    return;
  }else if (block_ptr < allocator->base || block_ptr > (void*)((char*)allocator->base + (allocator->block_size*allocator->nr_blocks))){
    return;
  }
  *((bool*)block_ptr-1) = false;

  while(*(bool*)allocator->tail == false){
    allocator->tail = ring_block_next_tail(allocator);
  }

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
  free(allocator->base);
  allocator->base = NULL;
  allocator->tail = NULL;
  allocator->head = NULL;
  allocator->block_size = 0;
  allocator->nr_blocks = 0;
}
