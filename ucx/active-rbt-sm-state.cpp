#include "active-rbt-sm-state.h"

#include "ucx-config.h"

#include <cassert>

namespace ucx::rbt {

RBTStateKind ActiveSMState::getStateKind() const {
  size_t KindOffset = offsetof(StateStructure, State);

  RBTStateKind KindValue;

  ucs_status_t status = ucp_get(ep, &KindValue, sizeof(RBTStateKind),
                                BaseAddress + KindOffset, SMKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return KindValue;
}

uint32_t ActiveSMState::getEvenOddBit() const {
  size_t BitOffset = offsetof(StateStructure, EvenOdd);

  uint32_t BitValue;

  ucs_status_t status =
      ucp_get(ep, &BitValue, sizeof(uint32_t), BaseAddress + BitOffset, SMKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return BitValue;
}

uint32_t ActiveSMState::getFlippedEvenOddBit() const {
  uint32_t Bit = getEvenOddBit();
  if (Bit == 0)
    return 1;
  return 0;
}

StateStructure ActiveSMState::getState() {
  StateStructure StateValue;

  ucs_status_t status =
      ucp_get(ep, &StateValue, sizeof(StateStructure), BaseAddress, SMKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return StateValue;
}

void ActiveSMState::setState(StateStructure SS) {
  StateStructure TheSS = SS;
  CommonMessage Msg;
  Msg.tag = Tag::FLUSH;
  Msg.Count = 1;
  Msg.Flush[0].Window = WindowKind::StateWindow;
  Msg.Flush[0].Offset = 0;
  Msg.Flush[0].Size = sizeof(StateStructure);

  ucs_status_t status =
      ucp_put(ep, &TheSS, sizeof(StateStructure), BaseAddress, SMKey);
  ucx::config::Comm->sendFlushMessage(Msg);

  if (status != UCS_OK)
    printf("failed to put (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);
}

void ActiveSMState::changeWoEvenOdd(RBTStateKind From, RBTStateKind To) {
  StateStructure SS = getState();

  assert(From != To);
  assert(SS.State == From);

  // printf("%s -> %s\n", StateKind2String(From).c_str(),
  //       StateKind2String(To).c_str());

  SS.State = To;
  setState(SS);
}

void ActiveSMState::changeWithEvenOdd(RBTStateKind From, RBTStateKind To) {
  StateStructure SS = getState();

  assert(From != To);
  assert(SS.State == From);

  // printf("%s -> %s\n", StateKind2String(From).c_str(),
  //       StateKind2String(To).c_str());

  if (SS.EvenOdd == 0)
    SS.EvenOdd = 1;
  else
    SS.EvenOdd = 0;

  SS.State = To;
  setState(SS);
}

void ActiveSMState::changeWithEvenOddSame(RBTStateKind From, RBTStateKind To) {
  StateStructure SS = getState();

  assert(SS.State == From);

  // printf("%s -> %s\n", StateKind2String(From).c_str(),
  //       StateKind2String(To).c_str());

  if (SS.EvenOdd == 0)
    SS.EvenOdd = 1;
  else
    SS.EvenOdd = 0;

  SS.State = To;
  setState(SS);
}

}; // namespace ucx::rbt
