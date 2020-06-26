#pragma once

#include "statistics.h"

#ifndef __APPLE__

#include "pmdk.h"

#include <libpmem.h>

#endif

#include <memory>

#if defined(USE_CLWB)

template <typename T> void write(T *dest, const T &val) {
  *dest = val;
  flush_clwb_nolog(dest, sizeof(T));
}

template <typename F> void flush(const F &f, FlushKind Kind) {
  f();
  predrain_memory_barrier();
}

[[maybe_unused]] static void flushRange(void *Addr, size_t len, FlushKind Kind) {
  flush_clwb_nolog(Addr, len);
  predrain_memory_barrier();
}

template <typename T> void flushObj(const T *t, FlushKind Kind) {
  flush_clwb_nolog(t, sizeof(T));
  predrain_memory_barrier();
}

#elif defined(USE_CLFLUSHOPT)

template <typename T> void write(T *dest, const T &val) {
  *dest = val;
  flush_clflushopt_nolog(dest, sizeof(T));
}

template <typename F> void flush(const F &f, FlushKind Kind) {
  f();
  predrain_memory_barrier();
}

template <typename T> void flushObj(const T *t, FlushKind Kind) {
  flush_clflushopt_nolog(t, sizeof(T));
  predrain_memory_barrier();
}

#elif defined(USE_CLFLUSH)

template <typename T> void write(T *dest, const T &val) {
  *dest = val;
  flush_clflush_nolog(dest, sizeof(T));
}

template <typename F> void flush(const F &f, FlushKind Kind) { f(); }

template <typename T> void flushObj(const T *t, FlushKind Kind) {
  flush_clflush_nolog(t, sizeof(T));
}

#elif defined(USE_SFENCE)

template <typename T> void write(T *dest, const T &val) { *dest = val; }

template <typename F> void flush(const F &f, FlushKind Kind) {
  f();
  predrain_memory_barrier();
}

template <typename T> void flushObj(const T *t, FlushKind Kind) { predrain_memory_barrier(); }

#elif defined(USE_NOFLUSH)

template <typename T> void write(T *dest, const T &val) { *dest = val; }

template <typename F> void flush(const F &f, FlushKind Kind) {
  f();
}

template <typename T> void flushObj(const T *t, FlushKind Kind) { }

#elif defined(USE_PMDK)

template <typename T> void write(T *dest, const T &val) {
  *dest = val;
  pmem_flush(dest, sizeof(T));
}

template <typename F> void flush(const F &f, FlushKind Kind) {
  f();
  pmem_drain();
}

template <typename T> void flushObj(const T *t, FlushKind Kind) {
  pmem_flush(t, sizeof(T));
  pmem_drain();
}

#elif defined(USE_STATISTICS)

template <typename T> void write(T *dest, const T &val) { *dest = val; }

template <typename F> void flush(const F &f, FlushKind Kind) { f(); statistics->addFlush(Kind); }

template <typename T> void flushObj(const T *t, FlushKind Kind) { statistics->addFlush(Kind);  }

#else

template <typename T> void write(T *dest, const T &val) { *dest = val; }

template <typename F> void flush(const F &f, FlushKind Kind) { f(); }

template <typename T> void flushObj(const T *t, FlushKind Kind) {}

#endif
