#include "dram-rbt-sm-state.h"

#include "recover.h"

#include <cassert>
#include <string>

namespace DRAM {

void SMState::change(StateStructure From, StateStructure To) {
  // if (isRecover)
    printf("%s -> %s\n", StateKind2String(From.State).c_str(),
           StateKind2String(To.State).c_str());
  // if (from != state)
  //printf("%s != %s -> %s\n", StateKind2String(State.State).c_str(),
  //       StateKind2String(From.State).c_str(), StateKind2String(To.State).c_str());

  assert(From == State);
  assert(From != To);
  State = To;
}

void SMState::changeWoEvenOdd(RBTStateKind From, RBTStateKind To) {
  assert(From != To);
  assert(State.State == From);

  State.State = To;
//     printf("%s -> %s\n", StateKind2String(From).c_str(),
//            StateKind2String(To).c_str());
}

void SMState::changeWoEvenOddSame(RBTStateKind From, RBTStateKind To) {
  assert(From == To);
  assert(State.State == From);

  State.State = To;
//    printf("%s -> %s\n", StateKind2String(From).c_str(),
//           StateKind2String(To).c_str());
}

void SMState::changeWithEvenOdd(RBTStateKind From, RBTStateKind To) {
  assert(From != To);
  assert(State.State == From);

  State.State = To;
  State.EvenOdd = getFlippedEvenOddBit();
//    printf("%s -> %s\n", StateKind2String(From).c_str(),
//           StateKind2String(To).c_str());
}

void SMState::changeWithEvenOddSame(RBTStateKind From, RBTStateKind To) {
  assert(From == To);
  assert(State.State == From);

  State.State = To;
  State.EvenOdd = getFlippedEvenOddBit();
//    printf("%s -> %s\n", StateKind2String(From).c_str(),
//           StateKind2String(To).c_str());
}

StateStructure SMState::getState() const { return State; }

RBTStateKind SMState::getStateKind() const { return State.State; }

void SMState::resetState() { State = {0, RBTStateKind::C0}; }

uint32_t SMState::getEvenOddBit() const { return State.EvenOdd; }

uint32_t SMState::getFlippedEvenOddBit() const {
  if (State.EvenOdd == 0)
    return 1;
  return 0;
}

} // namespace DRAM
