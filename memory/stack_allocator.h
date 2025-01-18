#ifndef MEM_STACK_ALLOCATOR
#define MEM_STACK_ALLOCATOR
#include "stdlib.h"
#include "stddef.h"
#include "stdbool.h"

typedef struct MEM_Stack_Frame{
  struct MEM_Stack_Frame* previous;
  void* start;
  size_t size;
}MEM_Stack_Frame;

typedef struct MEM_Stack_Allocator{
  void* base;
  size_t stack_size;
  MEM_Stack_Frame* top;

}MEM_Stack_Allocator;

MEM_Stack_Allocator MEM_stack_allocator_init(size_t size);

void* MEM_stack_allocator_alloc(MEM_Stack_Allocator* allocator, size_t size);

bool MEM_stack_allocator_realloc(MEM_Stack_Allocator* allocator, void* ptr, size_t new_size);

bool MEM_stack_allocator_free(MEM_Stack_Allocator* allocator, void* ptr);

bool MEM_stack_allocator_destroy(MEM_Stack_Allocator* allocator);
#ifdef UNITY_BUILD
#include "stack_allocator.c"
#endif //UNITY_BUILD

#endif
