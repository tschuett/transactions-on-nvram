#pragma once

#ifdef HAVE_PMDK

#include "state-kind.h"

#include <atomic>

namespace PMDK {
class SMState {
  //StateStructure *data = nullptr;

  std::atomic<uint64_t> *data = nullptr;
public:
  SMState();

  void change(StateStructure from, StateStructure to);
  StateStructure getState();

  void resetState();

  void changeWoEvenOdd(RBTStateKind From, RBTStateKind To);
  void changeWithEvenOdd(RBTStateKind From, RBTStateKind To);
  void changeWithEvenOddSame(RBTStateKind From, RBTStateKind To);

  RBTStateKind getStateKind() const;

  uint32_t getEvenOddBit() const;
  uint32_t getFlippedEvenOddBit() const;
};

} // namespace PMDK

#endif
