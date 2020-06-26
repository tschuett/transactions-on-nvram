#pragma once

#include <array>
#include <memory>

enum class FlushKind {
  SingleRotation = 0,
  DoubleRotation = 1,
  Recolor = 2,
  ColorFlip = 3,
  NewNode = 4,
  InitializeVariables = 5,
  FlushCommand = 6,
  ShuffleIterators = 7,
  UpdateRoot = 8,
  Misc = 9,
  RemoveNode = 10,
  StateChange = 11,
  Allocator = 12,
  InsertNewNode = 13,
  Loop = 14,
  BalancePoints = 15,
  BalanceFactors = 16,
  Balance = 17,
  FixParent = 18,
  PushStack = 19,
  PopStack = 20
};

class Statistics {
  std::array<uint64_t, 21> Flushes;
  unsigned FlushCounter = 0;

public:
  void addFlush(FlushKind Kind);

  void resetState();

  unsigned getFlushCounter() const;

  void dump() const;
};

extern std::unique_ptr<Statistics> statistics;
