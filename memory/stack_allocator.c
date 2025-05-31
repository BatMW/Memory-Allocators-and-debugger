#include "stack_allocator.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdbool.h"
#include "limits.h"


MEM_Stack_Allocator MEM_stack_allocator_init(size_t size){
  MEM_Stack_Allocator allocator = {};
  if(size <= 0){
    return allocator;
  }

  allocator.base = malloc(size);
  allocator.top = NULL;
  if(allocator.base != NULL){
    allocator.stack_size = size;
  }
  return allocator;
}

void* MEM_stack_allocator_alloc(MEM_Stack_Allocator* allocator, size_t size){
  if(allocator == NULL || size == 0){
    return NULL;
  }
  if(allocator->top == NULL){
    if(allocator->stack_size < sizeof(MEM_Stack_Frame) + size){
      return NULL;
    }else{
      MEM_Stack_Frame* new_top = (MEM_Stack_Frame*)allocator->base;
      new_top->size = size;
      new_top->previous = allocator->top;
      new_top->start = (void*)(new_top+1);
      allocator->top = new_top;
      return allocator->top->start;
    }
  }
  char* end_of_stack = (char*)allocator->base + allocator->stack_size;
  char* next_frame_position = (char*)allocator->top + allocator->top->size + sizeof(MEM_Stack_Frame);
  if((size_t)(end_of_stack - next_frame_position) < size + sizeof(MEM_Stack_Frame)){
    return NULL;
  }

  MEM_Stack_Frame* new_top = (MEM_Stack_Frame*)((char*)allocator->top + allocator->top->size + sizeof(MEM_Stack_Frame));
  new_top->size = size;
  new_top->previous = allocator->top;
  new_top->start = (void*)(new_top+1);
  allocator->top = new_top;
  return new_top->start;
}

void* MEM_stack_allocator_aligned_alloc(MEM_Stack_Allocator* allocator, size_t size, size_t align) {
  if (allocator == NULL || size == 0 || align == 0) {
    return NULL;
  }

  char* base = (char*)allocator->base;
  char* end_of_stack = base + allocator->stack_size;

  MEM_Stack_Frame* new_top = NULL;
  void* raw_mem = NULL;
  void* aligned_ptr = NULL;
  size_t padding = 0;

  if (allocator->top == NULL) {
    // First allocation
    new_top = (MEM_Stack_Frame*)allocator->base;
    raw_mem = (void*)(new_top + 1);
    aligned_ptr = align_address(raw_mem, align);
    padding = (size_t)((uintptr_t)aligned_ptr - (uintptr_t)raw_mem);

    size_t total_needed = sizeof(MEM_Stack_Frame) + padding + size;
    if ((size_t)(end_of_stack - (char*)new_top) < total_needed) {
      return NULL;
    }

    new_top->size = padding + size;
    new_top->previous = NULL;
    new_top->start = aligned_ptr;
    allocator->top = new_top;
    return aligned_ptr;
  }

  // Subsequent allocations
  char* next_frame_pos = (char*)allocator->top + sizeof(MEM_Stack_Frame) + allocator->top->size;
  new_top = (MEM_Stack_Frame*)next_frame_pos;
  raw_mem = (void*)(new_top + 1);
  aligned_ptr = align_address(raw_mem, align);
  padding = (size_t)((uintptr_t)aligned_ptr - (uintptr_t)raw_mem);

  size_t total_needed = sizeof(MEM_Stack_Frame) + padding + size;
  if ((size_t)(end_of_stack - (char*)new_top) < total_needed) {
    return NULL;
  }

  new_top->size = padding + size;
  new_top->previous = allocator->top;
  new_top->start = aligned_ptr;
  allocator->top = new_top;
  return aligned_ptr;
}

bool MEM_stack_allocator_realloc(MEM_Stack_Allocator* allocator, void* ptr, size_t new_size){
  if(allocator == NULL || ptr == NULL || allocator->base == NULL || allocator->top == NULL){
    return false;
  }
  if(ptr != allocator->top->start ){
    return false;
  }
  if((char*)ptr + new_size > (char*)allocator->base + allocator->stack_size ){
    return false;
  }
  allocator->top->size = new_size;
  return true;
}

bool MEM_stack_allocator_free(MEM_Stack_Allocator* allocator, void* ptr){
  if(allocator == NULL || ptr == NULL || allocator->top == NULL){
    return false;
  }
  if(ptr != allocator->top->start){
    return false;
  }
  allocator->top = allocator->top->previous;
  return true;
}

bool MEM_stack_allocator_destroy(MEM_Stack_Allocator* allocator){
  if(allocator == NULL){
    return false;
  }
  if(allocator->base == NULL){
    return false;
  }
  free(allocator->base);
  allocator->base = NULL;
  allocator->stack_size = 0;
  allocator->top = NULL;
  return true;
}


