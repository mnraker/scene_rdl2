// Copyright 2023-2024 DreamWorks Animation LLC and Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Platform.h"

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace scene_rdl2 {
namespace util {
// Intel: namespace embree {
  void* os_malloc(size_t bytes) 
  {
    char* ptr = (char*) VirtualAlloc(NULL,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if (ptr == NULL) throw std::bad_alloc();
    return ptr;
  }

  void* os_reserve(size_t bytes)
  {
    char* ptr = (char*) VirtualAlloc(NULL,bytes,MEM_RESERVE,PAGE_READWRITE);
    if (ptr == NULL) throw std::bad_alloc();
    return ptr;
  }

  void os_commit (void* ptr, size_t bytes) {
    VirtualAlloc(ptr,bytes,MEM_COMMIT,PAGE_READWRITE);
  }

  void os_shrink(void* ptr, size_t bytesNew, size_t bytesOld) 
  {
    size_t pageSize = 4096;
    if (bytesNew & (pageSize-1)) 
      bytesNew = (bytesNew+pageSize) & (pageSize-1);

    VirtualFree((char*)ptr+bytesNew,bytesOld-bytesNew,MEM_DECOMMIT);
  }

  void os_free(void* ptr, size_t bytes) {
    if (bytes == 0) return;
    VirtualFree(ptr,0,MEM_RELEASE);
  }

  void *os_realloc (void* ptr, size_t bytesNew, size_t bytesOld)
  {
    FATAL("not implemented");
  }

  double getSeconds() {
    LARGE_INTEGER freq, val;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&val);
    return (double)val.QuadPart / (double)freq.QuadPart;
  }
}
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////

#if defined(__UNIX__)

#include <sys/time.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

namespace scene_rdl2 {
namespace util {
// Intel: namespace embree {
  void* os_malloc(size_t bytes)
  {
    int flags = MAP_PRIVATE | MAP_ANON;
#if defined(__MIC__)
    if (bytes > 16*4096) {
      flags |= MAP_HUGETLB | MAP_POPULATE;
      bytes = (bytes+2*1024*1024-1)&ssize_t(-2*1024*1024);
    } else {
      bytes = (bytes+4095)&ssize_t(-4096);
    }
#endif
    char* ptr = (char*) mmap(0, bytes, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (ptr == NULL || ptr == MAP_FAILED) throw std::bad_alloc();
    return ptr;
  }

  void* os_reserve(size_t bytes)
  {
    int flags = MAP_PRIVATE | MAP_ANON | MAP_NORESERVE;
#if defined(__MIC__)
    if (bytes > 16*4096) {
      flags |= MAP_HUGETLB;
      bytes = (bytes+2*1024*1024-1)&ssize_t(-2*1024*1024);
    } else {
      bytes = (bytes+4095)&ssize_t(-4096);
    }
#endif
    char* ptr = (char*) mmap(0, bytes, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (ptr == NULL || ptr == MAP_FAILED) throw std::bad_alloc();
    return ptr;
  }

  void os_commit (void* ptr, size_t bytes) {
  }

  void os_shrink(void* ptr, size_t bytesNew, size_t bytesOld) 
  {
    size_t pageSize = 4096;
#if defined(__MIC__)
    if (bytesOld > 16*4096) pageSize = 2*1024*1024;
#endif
    if (bytesNew & (pageSize-1)) 
      bytesNew = (bytesNew+pageSize) & (pageSize-1);

    os_free((char*)ptr+bytesNew,bytesOld-bytesNew);
  }

  void os_free(void* ptr, size_t bytes) 
  {
    if (bytes == 0)
      return;

#if defined(__MIC__)
    if (bytes > 16*4096) {
      bytes = (bytes+2*1024*1024-1)&ssize_t(-2*1024*1024);
    } else {
      bytes = (bytes+4095)&ssize_t(-4096);
    }
#endif
    if (munmap(ptr,bytes) == -1) {
      throw std::bad_alloc();
    }
  }

  void* os_realloc (void* old_ptr, size_t bytesNew, size_t bytesOld)
  {
#if defined(__MIC__)
    if (bytesOld > 16*4096)
      bytesOld = (bytesOld+2*1024*1024-1)&ssize_t(-2*1024*1024);
    else
      bytesOld = (bytesOld+4095)&ssize_t(-4096);

    if (bytesNew > 16*4096)
      bytesNew = (bytesNew+2*1024*1024-1)&ssize_t(-2*1024*1024);
    else
      bytesNew = (bytesNew+4095)&ssize_t(-4096);

    char *ptr = (char*)mremap(old_ptr,bytesOld,bytesNew,MREMAP_MAYMOVE);

    if (ptr == NULL || ptr == MAP_FAILED) {
      perror("os_realloc ");
      throw std::bad_alloc();
    }
    return ptr;
#else
    FATAL("not implemented");
    return NULL;
#endif

  }


#if defined(__MIC__)

  static double getFrequencyInMHz()
  {
    struct timeval tvstart, tvstop;
    unsigned long long int cycles[2];
    
    gettimeofday(&tvstart, NULL);
    cycles[0] = rdtsc();
    gettimeofday(&tvstart, NULL);
    usleep(250000);
    gettimeofday(&tvstop, NULL);
    cycles[1] = rdtsc();
    gettimeofday(&tvstop, NULL);
  
    const unsigned long microseconds = ((tvstop.tv_sec-tvstart.tv_sec)*1000000) + (tvstop.tv_usec-tvstart.tv_usec);
    unsigned long mhz = (unsigned long) (cycles[1]-cycles[0]) / microseconds;

    //std::cout << "MIC frequency is " << mhz << " MHz" << std::endl;
    return (double)mhz;
  }

  static double micFrequency = getFrequencyInMHz();

#endif

  double getSeconds() {
#if !defined(__MIC__)
    struct timeval tp; gettimeofday(&tp,NULL);
    return double(tp.tv_sec) + double(tp.tv_usec)/1E6;
#else
    return double(rdtsc()) / double(micFrequency*1E6);
#endif
  }
}
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// Apple Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __APPLE__

#include <fenv.h>

int
feenableexcept(unsigned int excepts)
{
    fenv_t fenv;
    fegetenv(&fenv);

    excepts &= FE_ALL_EXCEPT;
#ifdef __aarch64__
    fenv.__fpcr &= ~(excepts << 8);
#else
    fenv.__control &= ~excepts;
    fenv.__mxcsr &= ~(excepts << 7);
#endif
    fesetenv(&fenv);

    return int(excepts); // TODO: doesn't look like there is failure handling return -1
}

void
fedisableexcept(unsigned int excepts)
{
    fenv_t fenv;
    fegetenv(&fenv);

#ifdef __aarch64__
    fenv.__fpcr |= ~(excepts << 8);
#else
    fenv.__control |= new_excepts;
    fenv.__mxcsr |= new_excepts << 7;
#endif

    fesetenv(&fenv);
}

int
fegetexcept()
{
    fenv_t fenv;
    fegetenv(&fenv);
    int excepts;
#ifdef __arm64__
    excepts = (fenv.__fpcr >> 8) & FE_ALL_EXCEPT;
#else
    excepts = fenv.__control & FE_ALL_EXCEPT;
#endif
    return int(excepts);
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// All Platforms
////////////////////////////////////////////////////////////////////////////////

// Intel: begin *****
/*
#include <stdlib.h>
#include <string.h>

namespace embree
{
  void* alignedMalloc(size_t size, size_t align)
  {
    if (size == 0) return NULL;
    char* base = (char*)malloc(size+align+sizeof(int));
    if (base == NULL) throw std::bad_alloc();

    char* unaligned = base + sizeof(int);
    char*   aligned = unaligned + align - ((size_t)unaligned & (align-1));
    ((int*)aligned)[-1] = (int)((size_t)aligned-(size_t)base);
    return aligned;
  }
  
  void alignedFree(const void* ptr) {
    if (ptr == NULL) return;
    int ofs = ((int*)ptr)[-1];
    free((char*)ptr-ofs);
  }
}
*/
// Intel: end *****

