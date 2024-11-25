#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "stdbool.h"
#include "ring_allocator.h"
#include "reset_allocator.h"
#include "assert.h"
#include "pool_allocator.h"
#include "stack_allocator.h"
void test_reset_allocator(void){
  printf("Starting reset allocator test.\n");
  size_t alloc_size = 1024;
  MEM_Reset_Allocator allocator = MEM_reset_allocator_init(alloc_size);
  assert(allocator.base != NULL);

  char* alloc1 = (char*)MEM_reset_allocator_alloc(&allocator, 512);
  assert(alloc1 != NULL);

  char* alloc2 = (char*)MEM_reset_allocator_alloc(&allocator, 600);
  assert(alloc2 == NULL);
  assert(allocator.failed_allocs == 1);

  MEM_reset_allocator_reset(&allocator);
  char* alloc3 = (char*)MEM_reset_allocator_alloc(&allocator, 256);
  assert(alloc1 == alloc3);
  assert(allocator.failed_allocs == 0);

  MEM_reset_allocator_destroy(&allocator);
  assert(allocator.base == NULL);
  printf("Reset allocator test done.\n");
}


void test_ring_allocator(void){
  printf("Starting ring allocator test.\n");
  size_t block_size = 31;
  size_t nr_blocks = 4;
  MEM_Ring_Block_Allocator allocator = MEM_ring_block_allocator_init(block_size, nr_blocks);
  assert(allocator.base != NULL);

  char* alloc0 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc0-1 == allocator.base);
  assert(*(bool*)(alloc0-1) == true);

  char* alloc1 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc1 != NULL);
  assert(*(bool*)(alloc1-1) == true);


  char* alloc2 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc2 != NULL);
  assert(*(bool*)(alloc2-1) == true);

  char* alloc3 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc3 != NULL);
  assert(*(bool*)(alloc0-1) == true);

  char* alloc4 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc4 == NULL); //Full buffer

  MEM_ring_block_allocator_free(&allocator, alloc2);
  assert(*(bool*)(alloc2-1) == false);
  assert(allocator.tail == allocator.base);

  MEM_ring_block_allocator_free(&allocator, alloc1);
  assert(*(bool*)(alloc1-1) == false);
  assert(allocator.tail == alloc0-1);

  MEM_ring_block_allocator_free(&allocator, alloc0);
  assert(*(bool*)(alloc0-1) == false);
  assert(allocator.tail == alloc3-1);

  alloc4 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc4-1 == allocator.base);
  //printf("alloc4ptr: %p , baseptr: %p \n", (void*)(alloc4-1), allocator.base);
  MEM_ring_block_allocator_destroy(&allocator);
  printf("Ring allocator test done.\n");
}

void test_pool_allocator(void){
  printf("Starting pool allocator test.\n");
  size_t block_size = 64;
  size_t nr_blocks = 128;
  MEM_Pool_Allocator allocator = MEM_pool_allocator_init(block_size, nr_blocks);
  assert(allocator.base != NULL);
  assert(allocator.start != NULL);
  assert((size_t)allocator.start % block_size == 0);
  printf("Allocating all ... \n");
  char* ptrs[nr_blocks];
  for(unsigned i=0; i<nr_blocks; ++i){
    ptrs[i] = MEM_pool_allocator_alloc(&allocator);
    assert(ptrs[i] != NULL);
    assert((size_t)ptrs[i] % block_size == 0);
  }
  //No more memory
  char* fail = MEM_pool_allocator_alloc(&allocator);
  assert(fail == NULL);


  char* old_spot_3 = ptrs[3];
  assert(MEM_pool_allocator_free(&allocator, ptrs[3]));
  assert(!MEM_pool_allocator_free(&allocator, ptrs[3]));

  ptrs[3]=NULL;
  assert(((*(size_t*)allocator.base) & ((size_t)1 << 3)) == 0);
  ptrs[3] = MEM_pool_allocator_alloc(&allocator);
  assert(old_spot_3 == ptrs[3]);
  
  printf("Freeing all ...\n");
  for(unsigned i=0; i<nr_blocks; ++i){
    assert(MEM_pool_allocator_free(&allocator, ptrs[i]));
    ptrs[i] = NULL;
  }
  char* ptrs2[nr_blocks*2] = {};
  printf("Randomly alloc/dealloc ... \n");
  srand(time(NULL));
  for(size_t i = 0; i< 10000000; ++i){
    //printf("i=%lu", i);
    size_t index = rand() % (nr_blocks*2);
    if(ptrs2[index] == NULL){
      if(((size_t*)allocator.base)[0] == ~((size_t)0) &&((size_t*)allocator.base)[1] == ~((size_t)0)){
        ptrs2[index] = MEM_pool_allocator_alloc(&allocator);
        assert(ptrs2[index] == NULL);
      }else{
        ptrs2[index] = MEM_pool_allocator_alloc(&allocator);
        assert(ptrs2[index] != NULL);
      }
    }else{
      assert(MEM_pool_allocator_free(&allocator, ptrs2[index]));
      ptrs2[index]=NULL;
    }
  }

  assert(MEM_pool_allocator_destroy(&allocator));
  printf("Pool allocator test done.\n");
}

void test_stack_allocator(void){
  printf("Starting stack allocator test...\n");
  MEM_Stack_Allocator allocator = MEM_stack_allocator_init(1024*1024); //1 MiB 
  assert(allocator.start != NULL);




  printf("Stack allocator test done.\n");
}

int main(void){
  test_reset_allocator();
  test_ring_allocator();
  test_pool_allocator();
  return 0;
}
