#pragma once

#include <emmintrin.h>
#include <x86intrin.h>

#include <cstdint>

#define FLUSH_ALIGN ((uintptr_t)64)
#define force_inline __attribute__((always_inline)) inline

static force_inline void predrain_memory_barrier(void) {
  _mm_sfence(); /* ensure CLWB or CLFLUSHOPT completes */
}

static force_inline void flush_clflush_nolog(const void *addr, size_t len) {
  uintptr_t uptr;

  /*
   * Loop through cache-line-size (typically 64B) aligned chunks
   * covering the given range.
   */
  for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
       uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN)
    _mm_clflush((char *)uptr);
}

/*
 * flush_clflushopt_nolog -- flush the CPU cache, using clflushopt
 */
static force_inline void flush_clflushopt_nolog(const void *addr, size_t len) {
  uintptr_t uptr;

  /*
   * Loop through cache-line-size (typically 64B) aligned chunks
   * covering the given range.
   */
  for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
       uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
    _mm_clflushopt((char *)uptr);
  }
}

/*
 * flush_clwb_nolog -- flush the CPU cache, using clwb
 */
static force_inline void flush_clwb_nolog(const void *addr, size_t len) {
  uintptr_t uptr;

  /*
   * Loop through cache-line-size (typically 64B) aligned chunks
   * covering the given range.
   */
  for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
       uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
#ifndef __APPLE__
    _mm_clwb((char *)uptr);
#endif
  }
}
