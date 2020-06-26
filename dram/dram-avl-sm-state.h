#pragma once

#include "state-kind.h"

namespace DRAM {

class AVLTSMState {
  AVLTStateStructure State = {0, AVLTStateKind::C0};

public:
  AVLTStateKind getStateKind() const;

  void changeWoEvenOdd(AVLTStateKind From, AVLTStateKind To);

  void resetState(){};
};

} // namespace DRAM
