#include "statistics.h"

#include <array>
#include <cassert>
#include <cinttypes>
#include <cstdio>

void Statistics::addFlush(FlushKind Kind) {
  FlushCounter++;
  Flushes[static_cast<int>(Kind)]++;
}

void Statistics::resetState() {
  FlushCounter = 0;
  for (unsigned i = 0; i < Flushes.size(); i++)
    Flushes[i] = 0;
}

unsigned Statistics::getFlushCounter() const { return FlushCounter; };

void Statistics::dump() const {
#ifdef __APPLE__
  printf("SingleRotation      = %llu\n", Flushes[0]);
  printf("DoubleRotation      = %llu\n", Flushes[1]);
  printf("Recolor             = %llu\n", Flushes[2]);
  printf("ColorFlip           = %llu\n", Flushes[3]);
  printf("NewNodeShort        = %llu\n", Flushes[4]);
  printf("InitializeVariables = %llu\n", Flushes[5]);
  printf("FlushCommand        = %llu\n", Flushes[6]);
  printf("ShuffleIterators    = %llu\n", Flushes[7]);
  printf("UpdateRoot          = %llu\n", Flushes[8]);
  printf("Misc                = %llu\n", Flushes[9]);
  printf("RemoveNode          = %llu\n", Flushes[10]);
  printf("StateChange         = %llu\n", Flushes[11]);
  printf("Allocator           = %llu\n", Flushes[12]);
  printf("InsertNewNode       = %llu\n", Flushes[13]);
  printf("Loop                = %llu\n", Flushes[14]);
  printf("BalancePoints       = %llu\n", Flushes[15]);
  printf("BalanceFactors      = %llu\n", Flushes[16]);
  printf("Balance             = %llu\n", Flushes[17]);
  printf("FixParent           = %llu\n", Flushes[18]);
  printf("PushStack           = %llu\n", Flushes[19]);
  printf("PopStack            = %llu\n", Flushes[20]);
#else
  printf("SingleRotation      = %lu\n", Flushes[0]);
  printf("DoubleRotation      = %lu\n", Flushes[1]);
  printf("Recolor             = %lu\n", Flushes[2]);
  printf("ColorFlip           = %lu\n", Flushes[3]);
  printf("NewNodeShort        = %lu\n", Flushes[4]);
  printf("InitializeVariables = %lu\n", Flushes[5]);
  printf("FlushCommand        = %lu\n", Flushes[6]);
  printf("ShuffleIterators    = %lu\n", Flushes[7]);
  printf("UpdateRoot          = %lu\n", Flushes[8]);
  printf("Misc                = %lu\n", Flushes[9]);
  printf("RemoveNode          = %lu\n", Flushes[10]);
  printf("StateChange         = %lu\n", Flushes[11]);
  printf("Allocator           = %lu\n", Flushes[12]);
  printf("InsertNewNode       = %lu\n", Flushes[13]);
  printf("Loop                = %lu\n", Flushes[14]);
  printf("BalancePoints       = %lu\n", Flushes[15]);
  printf("BalanceFactors      = %lu\n", Flushes[16]);
  printf("Balance             = %lu\n", Flushes[17]);
  printf("FixParent           = %lu\n", Flushes[18]);
  printf("PushStack           = %lu\n", Flushes[19]);
  printf("PopStack            = %lu\n", Flushes[20]);
#endif

  assert(Flushes.size() == 21);

  uint64_t Sum = 0;
  for (unsigned i = 0; i < Flushes.size(); i++)
    Sum += Flushes[i];

  printf("sum                 = %" PRIu64 "\n", Sum);
}

std::unique_ptr<Statistics> statistics;
