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
  if(allocator == NULL && size > 0){
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
  if(end_of_stack - next_frame_position < size + sizeof(MEM_Stack_Frame)){
    return NULL;
  }

  MEM_Stack_Frame* new_top = (MEM_Stack_Frame*)((char*)allocator->top + allocator->top->size + sizeof(MEM_Stack_Frame));
  new_top->size = size;
  new_top->previous = allocator->top;
  new_top->start = (void*)(new_top+1);
  allocator->top = new_top;
  return new_top->start;
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


