#ifndef MEM_POOL_ALLOCATOR
#define MEM_POOL_ALLOCATOR
#include "stddef.h"
#include "stdbool.h"
typedef struct MEM_Pool_Allocator{
  size_t block_size;
  size_t nr_blocks;
  size_t bit_mask_size;
  void* base;
  void* start;
} MEM_Pool_Allocator;

MEM_Pool_Allocator MEM_pool_allocator_init(size_t block_size, size_t nr_blocks);

void* MEM_pool_allocator_alloc(MEM_Pool_Allocator* allocator);

bool MEM_pool_allocator_free(MEM_Pool_Allocator* allocator, void* ptr);

bool MEM_pool_allocator_destroy(MEM_Pool_Allocator* allocator);

#endif
