#pragma once

#include "avlt-log.h"

namespace PMDK {
class AVLTLogState {
  AVLTLogStructure *data = nullptr;

public:
  AVLTLogState();

  void resetState();
  AVLTLogStructure *getLog() const { return data; }
};
} // namespace PMDK
