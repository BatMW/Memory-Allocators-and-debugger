#include "pool_allocator.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdbool.h"

#ifdef MEM_DBG
  #include "stdio.h"
  #include "assert.h"
#endif

struct MEM_Pool_Allocator MEM_pool_allocator_init(size_t block_size, size_t nr_blocks){
  struct MEM_Pool_Allocator allocator = {};
  #ifdef MEM_DBG
    printf("MEM_pool_allocator_init: block size: %lu, nr blocks: %lu\n", block_size, nr_blocks);

  #endif
  if(!(block_size > sizeof(struct MEM_Pool_Memory_Block)))return allocator;
  if((block_size & (block_size-1)) != 0)return allocator;
  if(block_size % sizeof(void*) != 0)return allocator; //aligned_alloc requirements

  allocator.base = (char*)aligned_alloc(block_size, block_size*nr_blocks);
  if(allocator.base == NULL){
    return allocator;
  }
  allocator.block_size = block_size;
  allocator.nr_blocks = nr_blocks;
  allocator.head = (struct MEM_Pool_Memory_Block*)allocator.base;

  char* block_iterator = allocator.base + block_size;
  struct MEM_Pool_Memory_Block* block_ptr = allocator.head;
  for(int i=0; i<nr_blocks-1; ++i){
    block_ptr->next = (struct MEM_Pool_Memory_Block*)block_iterator;
    block_iterator += block_size;
    block_ptr = block_ptr->next;
  }
  block_ptr->next = NULL;
  return allocator;
}

void* MEM_pool_allocator_alloc(struct MEM_Pool_Allocator* allocator){
  #ifdef MEM_DBG
    assert(allocator != NULL);
    assert(!(allocator->base == NULL));
  #endif
  if(allocator == NULL)return NULL;
  if(allocator->base == NULL || allocator->head == NULL)return NULL;
  void* ret = (void*)allocator->head;
  allocator->head = allocator->head->next;
  return ret;
}

bool MEM_pool_allocator_free(struct MEM_Pool_Allocator* allocator, void* ptr){
  #ifdef MEM_DBG
    assert(allocator != NULL);
    assert(!(allocator->base == NULL));
    assert(!((size_t)ptr > (size_t)((size_t)allocator->base + (allocator->nr_blocks*allocator->block_size))));
    assert(!((size_t)ptr % allocator->block_size != 0));
  #endif
  if(allocator == NULL)return false;
  if(allocator->base == NULL)return false;
  if ((char*)ptr < allocator->base || (char*)ptr >= (allocator->base + (allocator->nr_blocks * allocator->block_size))) return false;
  if ((size_t)ptr % allocator->block_size != 0) return false;
  struct MEM_Pool_Memory_Block* insert = (struct MEM_Pool_Memory_Block*)ptr;
  insert->next = allocator->head;
  allocator->head = insert;
  return true;
}

bool MEM_pool_allocator_destroy(struct MEM_Pool_Allocator* allocator){
  if(allocator == NULL)return false;
  if(allocator->base == NULL)return false;
  free(allocator->base);
  allocator->base = NULL;
  allocator->head = NULL;
  allocator->nr_blocks = 0;
  return true;
}


