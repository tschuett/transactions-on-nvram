#pragma once

#include "avlt-log.h"

namespace DRAM {

class AVLTLogState {
  AVLTLogStructure data;

public:
  AVLTLogStructure *getLog() { return &data; };

  void resetState(){};
};

} // namespace DRAM
