#include "thread_pool.h"
#include "../memory/pool_allocator.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

struct Thread_Pool_Queue* task_head;
struct Thread_Pool_Queue* task_tail;
struct MEM_Pool_Allocator task_allocator;
struct cthreads_mutex task_lock;
struct cthreads_semaphore nr_tasks;

enum thread_state{UNUSED, FREE, BUSY, CLOSING};
struct thread_node{
  struct cthreads_thread thread;
  volatile enum thread_state state;
  volatile bool close;
};
struct thread_node* thread_list;
size_t thread_list_size;


bool thread_pool_init(size_t max_size, size_t max_threads){
  static bool init = false;
  if(init == true) return true;
  task_allocator = MEM_pool_allocator_init(sizeof(struct Thread_Pool_Queue), max_size);
  if (task_allocator.base == NULL)return 1;
  thread_list = malloc(sizeof(struct thread_node)*max_threads);
  if (thread_list == NULL){
    MEM_pool_allocator_destroy(&task_allocator);
    return false;
  }
  for(int i=0; i<max_threads; ++i){
    thread_list[i].close = true;
    thread_list[i].state = UNUSED;
  }
  task_head = NULL;
  task_tail = NULL;
  cthreads_mutex_init(&task_lock, NULL);
  cthreads_sem_init(&nr_tasks, 0);
  thread_list_size = max_threads;
  return true;
}

static struct Thread_Pool_Task* thread_pool_retrieve(void){
  struct Thread_Pool_Task* ret = NULL;
  cthreads_mutex_lock(&task_lock);
  if(task_head == NULL) goto exit; //empty
  struct Thread_Pool_Queue* next = task_head->next;
  ret = task_head->task;
  MEM_pool_allocator_free(&task_allocator, task_head);
  if(task_head == task_tail){ //last node
    task_head = NULL;
    task_tail = NULL;
    goto exit;
  }
  task_head = next;
exit:
  cthreads_mutex_unlock(&task_lock);
  return ret;
}

bool thread_pool_add(struct Thread_Pool_Task* task){
  if(task == NULL) return false;
  cthreads_mutex_lock(&task_lock);
  struct Thread_Pool_Queue* new_task_node = (struct Thread_Pool_Queue*)MEM_pool_allocator_alloc(&task_allocator);
  if(new_task_node == NULL) goto error;
  new_task_node->task = task;
  if(task_head == NULL){
    task_head = new_task_node;
    task_head->next = NULL;
    task_tail = task_head;
    goto exit;
  }
  task_tail->next = new_task_node;
  new_task_node->next = NULL;
  task_tail = new_task_node;
exit:
  cthreads_sem_post(&nr_tasks);
  cthreads_mutex_unlock(&task_lock);
  return true;
error:
  cthreads_mutex_unlock(&task_lock);
  return false;
}


static void* worker_thread(void* void_thread_node){
  struct thread_node* thread_ptr = (struct thread_node*)void_thread_node;
  struct Thread_Pool_Task* curr_task = NULL;
  while(1){
    thread_ptr->state = FREE;
    cthreads_sem_wait(&nr_tasks); //sleep until there are tasks
    if(thread_ptr->close){
      break;
    }
    thread_ptr->state = BUSY;
    curr_task = thread_pool_retrieve();
    if(curr_task == NULL){
      continue;
    }
    printf("New task[%ld]\n", (thread_ptr - (struct thread_node*)thread_list));
    curr_task->ret = curr_task->func(curr_task->arg);
    cthreads_sem_post(curr_task->done);

  }
  thread_ptr->state = CLOSING;
  return NULL;
}

int thread_pool_start(size_t nr_threads){
  if(nr_threads == 0){
    return 0;
  }
  size_t cnt = 0;
  for(int i=0; i< thread_list_size; ++i){
    if(thread_list[i].state == UNUSED){
      thread_list[i].close = false;
      thread_list[i].state = FREE;
      cthreads_thread_create(&thread_list[i].thread, NULL, worker_thread, &thread_list[i], NULL);
      cnt++;
      if(cnt == nr_threads){
        return cnt;
      }
    }
  }
  return cnt;
}

void thread_pool_destroy(void){
  for(int i=0; i< thread_list_size; ++i){
    thread_list[i].close = true;
  }
  for(int i=0; i< thread_list_size; ++i){
    cthreads_sem_post(&nr_tasks);
    cthreads_sem_post(&nr_tasks);
  }
  for(int i=0; i< thread_list_size; ++i){
    if(thread_list[i].state != UNUSED){
      cthreads_thread_join(thread_list[i].thread, NULL);
      thread_list[i].state = UNUSED;
    }
  }
  free(thread_list);
  MEM_pool_allocator_destroy(&task_allocator);
}
