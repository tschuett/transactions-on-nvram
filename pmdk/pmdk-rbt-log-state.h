#pragma once

#include "rbt-log.h"

namespace PMDK {
class LogState {
  RBTLogStructure *data = nullptr;

public:
  LogState();

  void resetState();
  RBTLogStructure *getLog() const { return data; }
};
} // namespace PMDK
