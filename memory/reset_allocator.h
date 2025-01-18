#ifndef MEM_RESET_ALLOCATOR
#define MEM_RESET_ALLOCATOR
#include "stddef.h"

typedef struct MEM_Reset_Allocator{
  void* base;
  void* top;
  size_t size;
  size_t failed_allocs;
} MEM_Reset_Allocator;

/*
 * Tries to allocate 'size' bytes for the allocator.
 * MEM_Reset_Allocator.base == 0 if allocation was unsuccessful.
*/
MEM_Reset_Allocator MEM_reset_allocator_init(size_t size);

/*
 * Tries to allocate 'size' bytes of memory.
 * Returns NULL if unsuccessful.
*/
void* MEM_reset_allocator_alloc(MEM_Reset_Allocator* allocator, size_t size);

/*
 * Resets the 'top' pointer to equal base.
 * No pointers allocated before reset should not be used.
*/
void MEM_reset_allocator_reset(MEM_Reset_Allocator* allocator);

/*
 * Frees all memory and sets all pointers to NULL.
*/
void MEM_reset_allocator_destroy(MEM_Reset_Allocator* allocator);

#ifdef UNITY_BUILD
#include "reset_allocator.c"
#endif //UNITY_BUILD
#endif
