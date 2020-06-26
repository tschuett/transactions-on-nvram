#pragma once

#include "state-kind.h"

#include <ucp/api/ucp.h>

namespace ucx::rbt {

class ActiveSMState {
  ucp_rkey_h SMKey = nullptr;
  ucp_ep_h ep;
  uintptr_t BaseAddress;

public:
  ActiveSMState(ucp_ep_h ep) : ep(ep) {}

  void init(ucp_rkey_h SMKey_, uintptr_t BaseAddress_) {
    SMKey = SMKey_;
    BaseAddress = BaseAddress_;
  }

  void changeWoEvenOdd(RBTStateKind From, RBTStateKind To);
  void changeWithEvenOdd(RBTStateKind From, RBTStateKind To);
  void changeWithEvenOddSame(RBTStateKind From, RBTStateKind To);

  uint32_t getEvenOddBit() const;
  uint32_t getFlippedEvenOddBit() const;

  RBTStateKind getStateKind() const;

private:
  StateStructure getState();
  void setState(StateStructure);
};

} // namespace ucx::rbt
