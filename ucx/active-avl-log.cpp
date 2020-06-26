#include "active-avl-log.h"

#include "avlt-log.h"
#include "ucx-config.h"

#include <cstring>

namespace {

void putSyncOrAsync(ucp_ep_h ep, const void *buffer, size_t length,
                    uint64_t remote_addr, ucp_rkey_h rkey) {
  ucs_status_t status;
  if (ucx::config::AsyncPut) {
    status = ucp_put_nbi(ep, buffer, length, remote_addr, rkey);
    assert(status == UCS_OK or status == UCS_INPROGRESS);
  } else {
    status = ucp_put(ep, buffer, length, remote_addr, rkey);
    if (status != UCS_OK)
      printf("failed to put (%s)\n", ucs_status_string(status));
    assert(status == UCS_OK);
  }
}

} // namespace

namespace ucx::avlt {

void ActiveLog::init(ucp_rkey_h LogKey_, uintptr_t BaseAddress_) {
  LogKey = LogKey_;
  BaseAddress = BaseAddress_;
}

void ActiveLog::reset() {
  memset(&TheLog, 0, sizeof(AVLTLogStructure));
  TheLog.Root = Nullptr;
}

#define WRITEOPERATION(NAME, TYPE)                                             \
  void ActiveLog::write##NAME(TYPE NAME) {                                     \
    TYPE The##NAME = NAME;                                                     \
                                                                               \
    size_t NAME##Offset = offsetof(AVLTLogStructure, NAME);                    \
    putSyncOrAsync(ep, &The##NAME, sizeof(TYPE), BaseAddress + NAME##Offset,   \
                   LogKey);                                                    \
                                                                               \
    TheLog.NAME = NAME;                                                        \
  }

#define READOPERATION(NAME, TYPE)                                              \
  TYPE ActiveLog::read##NAME() {                                               \
    size_t NAME##Offset = offsetof(AVLTLogStructure, NAME);                    \
                                                                               \
    TYPE NAME##Value;                                                          \
                                                                               \
    ucs_status_t status = ucp_get(ep, &NAME##Value, sizeof(TYPE),              \
                                  BaseAddress + NAME##Offset, LogKey);         \
    if (status != UCS_OK)                                                      \
      printf("failed to get (%s)\n", ucs_status_string(status));               \
    assert(status == UCS_OK);                                                  \
                                                                               \
    return NAME##Value;                                                        \
  }

WRITEOPERATION(Key, uint64_t)
WRITEOPERATION(Value, uint64_t)
WRITEOPERATION(Root, NodeIndex)
WRITEOPERATION(Root2, NodeIndex)
WRITEOPERATION(Dir, Direction)
WRITEOPERATION(Balance, int8_t)
WRITEOPERATION(Balance2, int8_t)
WRITEOPERATION(Balance3, int8_t)
WRITEOPERATION(Inc, int8_t)
WRITEOPERATION(NextP, NodeIndex)
WRITEOPERATION(NextS, NodeIndex)
WRITEOPERATION(NextT, NodeIndex)
WRITEOPERATION(P, NodeIndex)
WRITEOPERATION(Q, NodeIndex)
WRITEOPERATION(S, NodeIndex)
WRITEOPERATION(T, NodeIndex)
WRITEOPERATION(Save, NodeIndex)
WRITEOPERATION(SaveChild, NodeIndex)
WRITEOPERATION(SP, NodeIndex)
WRITEOPERATION(N, NodeIndex)
WRITEOPERATION(NN, NodeIndex)

READOPERATION(Key, uint64_t)
READOPERATION(Value, uint64_t)
READOPERATION(Root, NodeIndex)
READOPERATION(Root2, NodeIndex)
READOPERATION(Dir, Direction)
READOPERATION(P, NodeIndex)
READOPERATION(Q, NodeIndex)
READOPERATION(S, NodeIndex)
READOPERATION(T, NodeIndex)
READOPERATION(Save, NodeIndex)
READOPERATION(NextP, NodeIndex)
READOPERATION(NextS, NodeIndex)
READOPERATION(NextT, NodeIndex)
READOPERATION(Balance, int8_t)
READOPERATION(Balance2, int8_t)
READOPERATION(Balance3, int8_t)
READOPERATION(Inc, int8_t)
READOPERATION(N, NodeIndex)
READOPERATION(NN, NodeIndex)
READOPERATION(SP, NodeIndex)
READOPERATION(SaveChild, NodeIndex)

} // namespace ucx::avlt
