#ifndef MEM_POOL_ALLOCATOR
#define MEM_POOL_ALLOCATOR
#include "stddef.h"
#include "stdbool.h"

struct MEM_Pool_Memory_Block{
  struct MEM_Pool_Memory_Block* next;
};

struct MEM_Pool_Allocator{
  size_t block_size;
  size_t nr_blocks;
  struct MEM_Pool_Memory_Block* head;
  char* base;
};

struct MEM_Pool_Allocator MEM_pool_allocator_init(size_t block_size, size_t nr_blocks);

void* MEM_pool_allocator_alloc(struct MEM_Pool_Allocator* allocator);

bool MEM_pool_allocator_free(struct MEM_Pool_Allocator* allocator, void* ptr);

bool MEM_pool_allocator_destroy(struct MEM_Pool_Allocator* allocator);

#ifdef UNITY_BUILD
  #include "pool_allocator.c"
#endif //UNITY_BUILD

#endif // MEM_POOL_ALLOCATOR
