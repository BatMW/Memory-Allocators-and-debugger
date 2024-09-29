#include "pool_allocator.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdbool.h"
#include "assert.h"
#include "limits.h"

inline static void toggle_bit(size_t* value, size_t bit){
  *value = *value ^ ((size_t)1<<bit);
}

MEM_Pool_Allocator MEM_pool_allocator_init(size_t block_size, size_t nr_blocks){
  MEM_Pool_Allocator allocator = {};
  
  if(!(block_size > 0))return allocator;
  if((block_size & (block_size-1)) != 0)return allocator;
  if(block_size % sizeof(void*) != 0)return allocator; //aligned_alloc requirements

  allocator.bit_mask_size = nr_blocks/sizeof(size_t)/CHAR_BIT;
  size_t bit_remainder =nr_blocks % sizeof(size_t)/CHAR_BIT;
  if(bit_remainder != 0){
    allocator.bit_mask_size++;
  }
  allocator.base = malloc(allocator.bit_mask_size*sizeof(size_t));
  if(allocator.base == NULL)return allocator;

  allocator.start = aligned_alloc(block_size, block_size*nr_blocks);
  if(allocator.start == NULL)return allocator;
  allocator.block_size = block_size;
  allocator.nr_blocks = nr_blocks;

  size_t* mask_ptr = (size_t*)allocator.base;
  for(size_t i =0; i< allocator.bit_mask_size; ++i){
    mask_ptr[i] = 0;
  }
  if(bit_remainder != 0){
    size_t total_bits = sizeof(size_t)*CHAR_BIT;
    *(mask_ptr+allocator.bit_mask_size-1) = 1;
    *(mask_ptr+allocator.bit_mask_size-1) = ~((*mask_ptr << (total_bits - bit_remainder))-1);
  }
  return allocator;
}

void* MEM_pool_allocator_alloc(MEM_Pool_Allocator* allocator){
  if(allocator == NULL)return NULL;
  if(allocator->base == NULL)return NULL;
  if(allocator->start == NULL)return NULL;
  size_t* mask_ptr = (size_t*)allocator->base;
  for(size_t i = 0; i< allocator->bit_mask_size; ++i){
    size_t bitmask = mask_ptr[i];
    if(~bitmask == 0){//All bits are set.
      continue;
    }
    for(size_t shift=0; shift < sizeof(size_t)*CHAR_BIT; ++shift){
      if(!(bitmask & ((size_t)1 << shift))){
        //found a free block at i*sizeof(size_t)+shift
        toggle_bit(mask_ptr+i, shift);
        return (void*)((char*)allocator->start + allocator->block_size * (i*sizeof(size_t)*CHAR_BIT + shift));
      }
      //bitmask = bitmask >> 1;
    }
  }
  return NULL;
}
bool MEM_pool_allocator_free(MEM_Pool_Allocator* allocator, void* ptr){
  if(allocator == NULL)return false;
  if(ptr == NULL)return false;
  if(allocator->base == NULL)return false;
  if(!((char*)ptr >= (char*)allocator->start && (char*)ptr < (char*)allocator->start + allocator->block_size*allocator->nr_blocks))return false;
  size_t byte_offset = (size_t)((char*)ptr - (char*)allocator->start);
  if(byte_offset % allocator->block_size != 0)return false;
  size_t block_offset = byte_offset / allocator->block_size;
  size_t bit_mask_size_t_offset = block_offset/ (sizeof(size_t)*CHAR_BIT);
  size_t bit_to_toggle_offset = block_offset % (sizeof(size_t)*CHAR_BIT);

  size_t* mask_ptr = (size_t*)allocator->base;
  if(!((((mask_ptr[bit_mask_size_t_offset] >> bit_to_toggle_offset))&(size_t)1) == (size_t)1))return false;
  //^The bit was never set, meaning it was never allocated, already deallocated by someone else, or some such version
  toggle_bit(mask_ptr+bit_mask_size_t_offset, bit_to_toggle_offset);
  return true;
}

bool MEM_pool_allocator_destroy(MEM_Pool_Allocator* allocator){
  if(allocator == NULL)return false;
  if(allocator->base == NULL)return false;
  free(allocator->base);
  free(allocator->start);
  allocator->base = NULL;
  allocator->start = NULL;
  allocator->nr_blocks = 0;
  allocator->block_size = 0;
  return true;
}


