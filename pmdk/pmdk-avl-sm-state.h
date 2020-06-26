#pragma once

#include "state-kind.h"

#include <atomic>

namespace PMDK {

class AVLTSMState {
  //AVLTStateStructure *data = nullptr;

  std::atomic<uint64_t> *data = nullptr;

public:
  AVLTSMState();

  void change(AVLTStateStructure from, AVLTStateStructure to);
  AVLTStateStructure getState();

  void resetState();

  void changeWoEvenOdd(AVLTStateKind From, AVLTStateKind To);
  void changeWithEvenOdd(AVLTStateKind From, AVLTStateKind To);
  void changeWithEvenOddSame(AVLTStateKind From, AVLTStateKind To);

  AVLTStateKind getStateKind() const;

  uint32_t getEvenOddBit() const;
  uint32_t getFlippedEvenOddBit() const;
};

} // namespace PMDK
