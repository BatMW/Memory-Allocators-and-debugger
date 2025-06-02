#define _GNU_SOURCE

#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "stdbool.h"
#include <string.h>
#include "ring_allocator.h"
#include "reset_allocator.h"
#include "assert.h"
#include "pool_allocator.h"
#include "stack_allocator.h"
#include "page_ring_buffer.h"

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
  size_t block_size = 32;
  size_t nr_blocks = 4;
  MEM_Ring_Block_Allocator allocator = MEM_ring_block_allocator_init(block_size, nr_blocks);
  assert(allocator.base != NULL);

  char* alloc0 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc0 == allocator.base);

  char* alloc1 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc1 != NULL);

  char* alloc2 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc2 != NULL);

  char* alloc3 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc3 != NULL);

  char* alloc4 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc4 == NULL); //Full buffer

  MEM_ring_block_allocator_free(&allocator, alloc0);
  assert(allocator.tail == 1);

  MEM_ring_block_allocator_free(&allocator, alloc1);
  assert(allocator.tail == 2);

  MEM_ring_block_allocator_free(&allocator, alloc2);
  assert(allocator.tail == 3);

  alloc4 = (char*)MEM_ring_block_allocator_alloc(&allocator);
  assert(alloc4 == allocator.base);
  //printf("alloc4ptr: %p , baseptr: %p \n", (void*)(alloc4-1), allocator.base);
  MEM_ring_block_allocator_destroy(&allocator);
  printf("Ring allocator test done.\n");
}

int check_ll(struct MEM_Pool_Allocator* allocator){
  printf("check linked list...\n");
  struct MEM_Pool_Memory_Block* checker = allocator->head;
  int sum=0;
  while(true){
    if(checker == NULL){
      break;
    }
    assert((size_t)checker % allocator->block_size == 0);
    size_t block_idx = ((size_t)checker - (size_t)allocator->base)/allocator->block_size;
    size_t next_idx = ((size_t)checker->next - (size_t)allocator->base)/allocator->block_size;
    printf("[%zu]->[%zu]: %zu\n", block_idx, next_idx, (size_t)checker->next);
    checker = checker->next;
    sum++;
  }
  return sum;
}

void test_pool_allocator(void){
  printf("Starting pool allocator test.\n");
  size_t block_size = 64;
  size_t nr_blocks = 128;
  struct MEM_Pool_Allocator allocator = MEM_pool_allocator_init(block_size, nr_blocks);
  assert(allocator.base != NULL);
  assert(allocator.head != NULL);
  assert((size_t)allocator.head % block_size == 0);

  printf("size calc: %zu\n", (size_t)((allocator.base + ((size_t)allocator.nr_blocks*allocator.block_size))-allocator.base)/block_size);
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
  ptrs[3]=NULL;
  ptrs[3] = MEM_pool_allocator_alloc(&allocator);
  assert(old_spot_3 == ptrs[3]);
  
  printf("Freeing all ...\n");
  for(unsigned i=0; i<nr_blocks; ++i){
    assert(MEM_pool_allocator_free(&allocator, ptrs[i]));
    ptrs[i] = NULL;
  }
  assert((size_t)check_ll(&allocator) == nr_blocks);

  char* ptrs2[nr_blocks*2] = {};
  printf("Randomly alloc/dealloc ... \n");
  srand(time(NULL));
  for(size_t i = 0; i< 10000000; ++i){
    //printf("i=%lu", i);
    size_t index = rand() % (nr_blocks*2);
    if(ptrs2[index] == NULL){
      if(allocator.head == NULL){
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
  printf("Freeing all ...\n");
  for(size_t i=0; i<nr_blocks*2; ++i){
    MEM_pool_allocator_free(&allocator, ptrs2[i]);
    ptrs2[i] = NULL;
  }
  size_t sum = check_ll(&allocator);
  printf("Sum of linked ptrs: %zu\n nr blocks: %zu\n", sum, nr_blocks );
   assert(sum == nr_blocks);
  //assert(MEM_pool_allocator_destroy(&allocator));
  printf("Pool allocator test done.\n");
}

void test_stack_allocator(void){
  printf("Starting stack allocator test...\n");
  struct MEM_Stack_Allocator allocator = MEM_stack_allocator_init(1024*1024); //1 MiB 
  assert(allocator.base != NULL);




  printf("Stack allocator test done.\n");
}

void test_page_ring_buffer(void){
  printf("Starting page ring buffer test.\n");
  fflush(stdout);
  MEM_Page_Ring_Buffer buf;
  buf = MEM_page_ring_buffer_init(1);
  assert(buf.base != NULL);
  assert(buf.start != NULL);
  assert(buf.start == buf.base + buf.p_size);
  printf("Page size: %zu\n", buf.page_size);
  fflush(stdout);
  char* b = (char*)buf.start;
  memset(b, 0, buf.p_size);
  assert(b[-1] == 0);

  char* str = "0123456789";
  strncpy(b + buf.p_size -3, str, 10);
  printf("Content of b-3: %s\n", b-3);
  printf("Content of b + buf.p_size -3: %s\n", b + buf.p_size -3);
  fflush(stdout);

  assert(strcmp(b-3, b + buf.p_size -3) == 0);

  MEM_page_ring_buffer_destroy(&buf);
  printf("Page ring buffer test done.\n");
  fflush(stdout);

}


int main(void){
  test_reset_allocator();
  test_ring_allocator();
  test_pool_allocator();
  test_stack_allocator();
  test_page_ring_buffer();
  return 0;
}
