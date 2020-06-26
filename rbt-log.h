#pragma once

#include "tree.h"

#include <array>
#include <cstdint>

struct alignas(64) Iterators {
  NodeIndex QP;
  Direction LastDir;
  Direction Dir;
  NodeIndex Tp;
  NodeIndex GrandP;
  NodeIndex ParentP;
  NodeIndex Q2P;
  // remove
  NodeIndex Fp = Nullptr;
};

struct alignas(64) RedoLog {
  Node TmpNode;
  // for remove
  NodeIndex Sp = Nullptr;
  Direction Dir3 = Direction::Unknown;

  // the log
  Direction Dir2 = Direction::Unknown;
  NodeIndex SaveP = Nullptr;
  NodeIndex SaveChildP = Nullptr;

  NodeIndex Parent2P = Nullptr;
  NodeIndex Root2P = Nullptr;
};

struct alignas(64) RBTLogStructure {
  Iterators Iterator[2];
  RedoLog Log;
  uint64_t Key = 0;
  uint64_t Value = 0;
  NodeIndex Root = Nullptr;
};
