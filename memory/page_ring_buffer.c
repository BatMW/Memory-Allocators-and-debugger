#define _GNU_SOURCE
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#endif
#include "page_ring_buffer.h"
#include <stddef.h>

/*
typedef struct MEM_Page_Ring_Buffer{
  void* base;     // base allocation
  void* start;   // start of read-writeable area (base + p_size)
  size_t p_size; //physical size
  size_t v_size; //virtual size
  size_t page_size;
}MEM_Page_Ring_Buffer;
*/

#if defined (_WIN32)
MEM_Page_Ring_Buffer MEM_page_ring_buffer_init(size_t physical_nr_pages) {
  MEM_Page_Ring_Buffer buf = {0};

  if (physical_nr_pages == 0)
      goto error1;

  SYSTEM_INFO si;
  GetSystemInfo(&si);
  size_t page_size = (size_t)si.dwPageSize;
  size_t p_size = physical_nr_pages * page_size;
  size_t v_size = 3 * p_size;

  HANDLE mapping = CreateFileMappingW(
    INVALID_HANDLE_VALUE,        // use the system page file
    NULL,                        // default security
    PAGE_READWRITE,              // read/write access
    (DWORD)((p_size >> 32) & 0xFFFFFFFF), // size high
    (DWORD)(p_size & 0xFFFFFFFF),        // size low
    NULL                         // no name
  );

  if (mapping == NULL)
    goto error1;

  // Reserve 3 * p_size virtual address range
  void* base = VirtualAlloc2(
    GetCurrentProcess(),
    NULL,
    v_size,
    MEM_RESERVE,
    PAGE_NOACCESS,
    NULL,
    0
  );

  if (base == NULL) {
    goto error2;
  }

  for (int i = 0; i < 3; ++i) {
    void* addr = (char*)base + i * p_size;

    void* view = MapViewOfFile3(
      mapping,
      GetCurrentProcess(),
      addr,
      0,
      p_size,
      MEM_REPLACE_PLACEHOLDER,
      PAGE_READWRITE,
      NULL,
      0
    );

    if (view == NULL) {
      for (int j = 0; j < i; ++j)
        UnmapViewOfFile((char*)base + j * p_size);
      goto error3;
    }
  }

  buf.base = base;
  buf.start = (char*)base + p_size;
  buf.p_size = p_size;
  buf.v_size = v_size;
  buf.page_size = page_size;

  CloseHandle(mapping); // mapping can be closed after views are mapped

  return buf;

error3:

  VirtualFree(base, 0, MEM_RELEASE);
error2:
  CloseHandle(mapping);
error1:
  return buf;
}

void MEM_page_ring_buffer_destroy(MEM_Page_Ring_Buffer* allocator) {
  if (!allocator || !allocator->base)
    return;

  for (int i = 0; i < 3; ++i)
    UnmapViewOfFile((char*)allocator->base + i * allocator->p_size);

  VirtualFree(allocator->base, 0, MEM_RELEASE);
  allocator->base = NULL;
  allocator->start = NULL;
  allocator->p_size = 0;
  allocator->v_size = 0;
}
#else
MEM_Page_Ring_Buffer MEM_page_ring_buffer_init(size_t physical_nr_pages){
  MEM_Page_Ring_Buffer buf;
  buf.base = NULL;
  buf.start = NULL;
  buf.p_size = 0;
  buf.v_size = 0;
  buf.page_size = 0;

  if(physical_nr_pages == 0) return buf;
  size_t page_size = 0;
  long int tmp = sysconf(_SC_PAGESIZE);
  if(tmp <0) return buf;
  page_size = (size_t)tmp;

  size_t p_size = physical_nr_pages * page_size;
  size_t v_size = 3*p_size;

/*  See man mem_fd(2) (MFD_CLOEXEC) and man open(2) (O_CLOEXEC)
  * MFD_CLOEXEC prevents a new process from inhereting the file descriptor in the open file table
*/
  int mem_fd = memfd_create("ring_buffer", MFD_CLOEXEC);
  if (mem_fd == -1) {
      perror("memfd_create");
      goto error1;
  }

  if (ftruncate(mem_fd, (off_t)p_size) == -1) {
    goto error2;
  }

  void* base = mmap(NULL, v_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (base == MAP_FAILED) {
    perror("mmap reserve");
    goto error2;
  }

  for (int i = 0; i < 3; ++i) {
    void* addr = (char*)base + i * p_size;
    void* map = mmap(addr, p_size, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_FIXED, mem_fd, 0);
    if (map == MAP_FAILED) {
      perror("mmap map segment");
      goto error3;
    }
  }

  buf.base = base;
  buf.start = (char*)base + p_size;
  buf.p_size = p_size;
  buf.v_size = v_size;
  buf.page_size = page_size;

  close(mem_fd);
  return buf;

error3:
  munmap(base, v_size);
error2:
  close(mem_fd);
error1:
  return buf;
}

void MEM_page_ring_buffer_destroy(MEM_Page_Ring_Buffer* allocator){
  if (!allocator || !allocator->base) return;
  munmap(allocator->base, allocator->v_size);
  allocator->base = NULL;
  allocator->start = NULL;
  allocator->p_size = 0;
  allocator->v_size = 0;
}
#endif

