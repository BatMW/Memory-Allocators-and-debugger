#include "memdbg.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdio.h"


#ifdef MEMDEBUG
  #undef MEMDEBUG //Avoid overloading of internal malloc etc
#endif
/*
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
*/

void* CLIBS_memdbg_malloc(size_t size, const char* file,unsigned int line){

}
void* CLIBS_memdbg_calloc(size_t count, size_t size, const char* file,unsigned int line){}
void* CLIBS_memdbg_realloc(void* ptr, size_t size, const char* file,unsigned int line){}
void* CLIBS_memdbg_aligned_alloc(size_t align, size_t size, const char* file,unsigned int line){}
void  CLIBS_memdbg_free(void* ptr, const char* file,unsigned int line){}

void CLIBS_memdbg_log(const char* msg){}

#define MEMDEBUG
