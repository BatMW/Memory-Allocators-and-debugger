#ifndef MEM_PAGE_RING_BUFFER
#define MEM_PAGE_RING_BUFFER
#define _GNU_SOURCE
#if defined(_WIN32)
//#define _WIN32_WINNT 0x0A00
#include <windows.h>
#else
#define _GNU_SOURCE
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <stddef.h>

typedef struct MEM_Page_Ring_Buffer{
  void* base;     // base allocation
  void* start;   // start of read-writeable area
  size_t p_size; //physical size
  size_t v_size; //virtual size
  size_t page_size;
#if defined (_WIN32)
  HANDLE hMap;
#endif
}MEM_Page_Ring_Buffer;

MEM_Page_Ring_Buffer MEM_page_ring_buffer_init(size_t physical_nr_pages);

void MEM_page_ring_buffer_destroy(MEM_Page_Ring_Buffer* allocator);



#ifdef UNITY_BUILD
#include "page_ring_buffer.c"
#endif

#endif
