#pragma once

#include "tree.h"

#include <cstdint>

struct AVLTLogStructure {
  uint64_t Key = 0;
  uint64_t Value = 0;
  NodeIndex Root = Nullptr;
  NodeIndex S = Nullptr; // Place to rebalance
  NodeIndex T = Nullptr; // and parent
  NodeIndex P = Nullptr; // Iterator
  NodeIndex Q = Nullptr; // and save pointer
  Direction Dir = Direction::Unknown;
  int8_t Inc = 0;
  int8_t Balance = 0;
  NodeIndex NextP = Nullptr;
  NodeIndex NextS = Nullptr;
  NodeIndex NextT = Nullptr;
  int8_t Balance2 = 0;
  int8_t Balance3 = 0;
  NodeIndex SP = Nullptr;
  NodeIndex N = Nullptr;
  NodeIndex NN = Nullptr;
  NodeIndex Save = Nullptr;
  NodeIndex SaveChild = Nullptr;
  NodeIndex Root2 = Nullptr;
  /* remove */
  NodeIndex It;
  NodeIndex OldIt;
  NodeIndex NewItDown;
  NodeIndex NewIt;
  NodeIndex NewItUp;
  NodeIndex OldNewIt;
  NodeIndex OldNewItUp;
  //NodeIndex Up[32];
  //Direction Upd[32];
  NodeIndex SaveParent = Nullptr;
  Direction SaveParentDir = Direction::Unknown;
  bool Done;
  int8_t Top;
  int8_t OldTop;
  Direction Dir2 = Direction::Unknown;
  Direction Dir3 = Direction::Unknown;
  NodeIndex Heir;
  NodeIndex OldHeir;
  NodeIndex Foo;                 // rename
  Direction FooDir;              // rename
};
