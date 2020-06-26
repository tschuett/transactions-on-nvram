#include "dram-avl-sm-state.h"

#include <cassert>
#include <string>

namespace DRAM {

AVLTStateKind AVLTSMState::getStateKind() const { return State.State; }

void AVLTSMState::changeWoEvenOdd(AVLTStateKind From, AVLTStateKind To) {
  //printf("%s -> %s\n", StateKind2String(From).c_str(),
  //       StateKind2String(To).c_str());

  assert(From != To);
  assert(State.State == From);

  State.State = To;
}

} // namespace DRAM
