#define _GNU_SOURCE
#if defined(_WIN32)
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
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
void* find_aligned_free_region(size_t size) {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  uintptr_t alloc_gran = si.dwAllocationGranularity;

  uintptr_t start_addr = (uintptr_t)si.lpMinimumApplicationAddress;
  uintptr_t end_addr = (uintptr_t)si.lpMaximumApplicationAddress;

  uintptr_t addr = start_addr;

  while (addr < end_addr) {
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T result = VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi));
    if (result == 0) {
      break;
    }

    if (mbi.State == MEM_FREE) {
      uintptr_t region_base = (uintptr_t)mbi.BaseAddress;
      uintptr_t region_size = (uintptr_t)mbi.RegionSize;

      uintptr_t aligned_start = (region_base + alloc_gran - 1) & ~(alloc_gran - 1);

      if (aligned_start + size <= region_base + region_size &&
        aligned_start >= region_base) {
        return (void*)aligned_start;
      }
    }

    uintptr_t next = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    if (next <= addr) {
        break;
    }
    addr = next;
  }

  return NULL;
}

MEM_Page_Ring_Buffer MEM_page_ring_buffer_init(size_t physical_nr_pages) {
  MEM_Page_Ring_Buffer buf = {0};
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  size_t page_size = si.dwAllocationGranularity;

  if (physical_nr_pages == 0) return buf;

  size_t p_size = physical_nr_pages * page_size;
  size_t v_size = 3 * p_size;

  DWORD size_high = (DWORD)((p_size >> 32) & 0xFFFFFFFF);
  DWORD size_low = (DWORD)(p_size & 0xFFFFFFFF);
  printf("size of p_size: %zu\n", p_size);
  printf("size high: %lu\n", size_high);
  printf("size low: %lu\n", size_low);
  HANDLE hMap = CreateFileMapping(
    INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    size_high,
    size_low,
    NULL
  );

  if (hMap == NULL) {
    printf("CreateFileMapping failed: %lu\n", GetLastError());
    return buf;
  }

  // Reserve virtual address space of 3x size with no access
  const int max_attempts = 10000;
  for (int attempt = 0; attempt < max_attempts; ++attempt) {
    void* base = find_aligned_free_region(v_size);
    printf("Found suitable region\n");
    if (base == NULL) {
      printf("No suitable free region found\n");
      CloseHandle(hMap);
      return buf;
    }

    void* views[3];
    bool success = true;
    for (int i = 0; i < 3; ++i) {
      void* addr = (char*)base + i * p_size;
      views[i] = MapViewOfFileEx(
        hMap,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        p_size,
        addr
      );
      if (views[i] == NULL) {
        printf("MapViewOfFileEx failed at index %d: %lu\n", i, GetLastError());
        // Cleanup all mapped views so far
        for (int j = 0; j < i; ++j) {
            UnmapViewOfFile(views[j]);
        }
        break;
      }
    }

    if (success) {
      buf.base = base;
      buf.start = (char*)base + p_size;
      buf.p_size = p_size;
      buf.v_size = 3 * p_size;
      buf.page_size = page_size;
      buf.hMap = hMap;
      return buf;
    }
  }
  printf("Failed to map 3 contiguous views after %d attempts\n", max_attempts);
  CloseHandle(hMap);
  return buf;
}

void MEM_page_ring_buffer_destroy(MEM_Page_Ring_Buffer* buf) {
  if (!buf || !buf->base) return;
  for (int i = 0; i < 3; ++i) {
    VirtualFree((char*)buf->base + i * buf->p_size, buf->p_size, MEM_RELEASE);
  }
  VirtualFree(buf->base, 0, MEM_RELEASE);
  if (buf->hMap) {
    CloseHandle(buf->hMap);
  }

  buf->base = NULL;
  buf->start = NULL;
  buf->p_size = 0;
  buf->v_size = 0;
  buf->page_size = 0;
  buf->hMap = NULL;
}
#else //Linux implementation
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

