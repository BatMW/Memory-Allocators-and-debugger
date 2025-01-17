#include "stdio.h"
#include "cthreads.h"
#include "thread_pool.h"

#define NR_TASKS 1024
#define NR_THREADS 7

struct cthreads_mutex mx_stdout;

struct test_func_arg_list{
  int x;
  int y;
  int z;
};

void* test_func(void* args){
  struct test_func_arg_list* type_args = (struct test_func_arg_list*)args;
  cthreads_mutex_lock(&mx_stdout);
  printf("Arguments:\nx: %d y: %d z: %d\n", type_args->x, type_args->y, type_args->z);
  cthreads_mutex_unlock(&mx_stdout);
  return NULL;
}

int main(int argc, char* argv[]){

  thread_pool_init(1024, NR_THREADS);

  struct Thread_Pool_Task tasks[NR_TASKS];
  struct test_func_arg_list arg_list[NR_TASKS];
  struct cthreads_semaphore done_list[NR_TASKS];
  for(int i=0; i<NR_TASKS; ++i){
    arg_list[i].x = (i+1)*i;
    arg_list[i].y = (i+1)*i + 1;
    arg_list[i].z = (i+1)*i + 2;
    tasks[i].func = &test_func;
    tasks[i].arg = (void*)&arg_list[i];
    tasks[i].done = &done_list[i];
    cthreads_sem_init(tasks[i].done, 0);
    thread_pool_add(&tasks[i]);
  }
  thread_pool_start(NR_THREADS);
  
  for(int i =0; i<NR_TASKS; ++i){
    cthreads_sem_wait(tasks[i].done);
  }
  
  for(int a=0; a < 100; ++a){
    for(int i=0; i<NR_TASKS; ++i){
      cthreads_sem_init(tasks[i].done, 0);
      thread_pool_add(&tasks[i]);
    }
    for(int i =0; i<NR_TASKS; ++i){
      cthreads_sem_wait(tasks[i].done);
    }
  }
  printf("Destroying stuff.\n");

  thread_pool_destroy();
  printf("All done.\n");
  return 0;
}
