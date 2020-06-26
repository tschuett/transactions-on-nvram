#pragma once

#include "rbt-log.h"

namespace DRAM {

class LogState {
  RBTLogStructure data;

public:
  LogState();

  void resetState();
  RBTLogStructure *getLog()  { return &data; }
};

} // namespace DRAM
