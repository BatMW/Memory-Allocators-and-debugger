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
  #ifdef MEM_DBG
    struct MEM_Pool_Memory_Block* checker = allocator.head;
    int sum=0;
    while(true){
      if(checker == NULL){
        break;
      }
      assert((size_t)checker % allocator.block_size == 0);
      size_t block_idx = ((size_t)checker - (size_t)allocator.base)/allocator.block_size;
      size_t next_idx = ((size_t)checker->next - (size_t)allocator.base)/allocator.block_size;
      printf("[%lu]->[%lu]: %lu\n", block_idx, next_idx, (size_t)checker->next);
      checker = checker->next;
      sum++;
    }
    printf("sum of blocks: %d\n", sum);
  #endif
  return allocator;
}

void* MEM_pool_allocator_alloc(struct MEM_Pool_Allocator* allocator){
  if(allocator == NULL)return NULL;
  if(allocator->base == NULL || allocator->head == NULL)return NULL;
  void* ret = (void*)allocator->head;
  allocator->head = allocator->head->next;
  return ret;
}

bool MEM_pool_allocator_free(struct MEM_Pool_Allocator* allocator, void* ptr){
  if(allocator == NULL)return false;
  if(allocator->base == NULL)return false;
  if((char*)ptr < allocator->base || (size_t)ptr > (size_t)(allocator->base + (allocator->nr_blocks*allocator->block_size))) return false;
  if((size_t)ptr % allocator->block_size != 0) return false;
 
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


