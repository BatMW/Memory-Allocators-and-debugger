#ifndef THREAD_POOL
#define THREAD_POOL
#include "cthreads.h"
#include <stdbool.h>

//typedef func_ptr_t (void*)(void*) // <==> void* (*func)(void*)

struct Thread_Pool_Task{
  void* (*func)(void*);
  void* arg;
  void* ret;
  struct cthreads_semaphore* done;
};

struct Thread_Pool_Queue{
  struct Thread_Pool_Task* task;
  struct Thread_Pool_Queue* next;
};


bool thread_pool_init(size_t max_size, size_t max_threads);

bool thread_pool_add(struct Thread_Pool_Task* task);

int thread_pool_start(size_t nr_threads);

void thread_pool_destroy(void);

#endif
