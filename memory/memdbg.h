#ifndef CLIBS_ERROR_MEMORY
#define CLIBS_ERROR_MEMORY

#include "stdlib.h"
#include "stddef.h"
#include "stdio.h"

#ifdef MEM_DBG
#define malloc(size) memdbg_malloc(size, __FILE__, __LINE__)
#define calloc(count, size) memdbg_calloc(count, size, __FILE__, __LINE__)
#define realloc(ptr, size) memdbg_realloc(ptr, size, __FILE__, __LINE__)
#define aligned_alloc(align, size) memdbg_aligned_alloc(align, size, __FILE__, __LINE__)
#define free(ptr) memdbg_free(ptr, __FILE__, __LINE__)

/*
 * TODO: Overload all functions in own memory allocators
 */

enum MEMDBG_Memory_Status{VALID, REALLOC_NEW, DESTROYED};

struct MEMDBG_Memory_Allocation{
  size_t size;
  unsigned int line;
  enum MEMDBG_Memory_Status status;
  char* data;
};

struct MEMDBG_File_Memory_Data{
  char* filename;
  CLIBS_Memory_Allocation* allocations; //dynamic array?
};

struct MEMDBG_Memory_Data{
  struct MEMDBG_File_Memory_Data* file_data;
  size_t allocation_count;
  int log_fd;
};

void* memdbg_malloc(size_t size, const char* file,unsigned int line);
void* memdbg_calloc(size_t count, size_t size, const char* file,unsigned int line);
void* memdbg_realloc(void* ptr, size_t size, const char* file,unsigned int line);
void* memdbg_aligned_alloc(size_t align, size_t size, const char* file,unsigned int line);
void  memdbg_free(void* ptr, const char* file,unsigned int line);

void memdbg_log(const char* msg);
#ifdef UNITY_BUILD
  #include "memdbg.c"
#endif //UNITY_BUILD

#endif //DEBUG

#endif //CLIBS_ERROR_MEMORY


