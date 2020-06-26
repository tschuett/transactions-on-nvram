#pragma once

#include "state-kind.h"

#include <cstdint>

namespace DRAM {

class SMState {
  StateStructure State = {0, RBTStateKind::C0};

public:
  void change(StateStructure From, StateStructure To);

  void changeWoEvenOdd(RBTStateKind From, RBTStateKind To);
  void changeWoEvenOddSame(RBTStateKind From, RBTStateKind To);
  void changeWithEvenOdd(RBTStateKind From, RBTStateKind To);
  void changeWithEvenOddSame(RBTStateKind From, RBTStateKind To);

  StateStructure getState() const;

  RBTStateKind getStateKind() const;

  void resetState();

  uint32_t getEvenOddBit() const;
  uint32_t getFlippedEvenOddBit() const;
};
} // namespace DRAM
