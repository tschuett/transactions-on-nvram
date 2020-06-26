#pragma once

namespace {
template <typename Last> void flushArgs(Last last) { last.flush(); }

template <class First, class... Rest>
void flushArgs(First first, Rest... rest) {
  first.flush();
  flushArgs(rest...);
}

} // namespace

template <typename AType, typename VType> class WriteOp {
  AType Addr;
  VType Value;

public:
  WriteOp(AType address, VType value) : Addr(address), Value(value) {}

  void flush() { ::write(Addr.getAddress(), Value.readValue()); }
  //void write() { *Addr.getAddress() = Value.readValue(); }
};

template <class... Types> void flushOp(FlushKind Kind, Types... args) {
  flushArgs(args...);
#ifndef __APPLE__
  predrain_memory_barrier();
#endif
  // drain();
  statistics->addFlush(Kind);
}
