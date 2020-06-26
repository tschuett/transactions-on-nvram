#pragma once

#include "write-dsl/window-kind.h"

#include <ucp/api/ucp.h>

#include <string>

enum class Tag : uint8_t { FLUSH, FLUSH_ACK, ASSERT, ASSERT_ACK, KEYXCHG };

struct __attribute__((__packed__)) FlushParameters {
  WindowKind Window;
  uint32_t Offset;
  uint8_t Size;
};

struct __attribute__((__packed__)) CommonMessage {
  static constexpr size_t Slots = 9;
  Tag tag;
  uint8_t Count;
  FlushParameters Flush[Slots];
};


struct __attribute__((__packed__)) KeyExchangeMessage {
  static constexpr size_t KeySize = 50;
  Tag tag;
  uint8_t NodesKey[KeySize];
  uint8_t CurrentNodeKey[KeySize];
  uint8_t SMKey[KeySize];
  uint8_t LogKey[KeySize];
  uintptr_t BaseAddressNodes;
  uintptr_t BaseAddressCurrentNode;
  uintptr_t BaseAddressSM;
  uintptr_t BaseAddressLog;
};

typedef struct ucx_server_ctx {
  ucp_ep_h ep;
} ucx_server_ctx_t;

const uint16_t ServerPort = 13337;

extern void initUCX(ucp_context_h *ucp_context, ucp_worker_h *ucp_worker);

extern void closeEp(ucp_worker_h ucp_worker, ucp_ep_h ep);

extern void wait(ucp_worker_h ucp_worker, ucs_status_t status,
                 uint8_t *request);

extern void waitTagRecv(ucp_worker_h ucp_worker, ucs_status_ptr_t status);

extern void waitTagSend(ucp_worker_h ucp_worker, ucs_status_ptr_t status);

extern void checkConsistency(ucp_ep_h ep, ucp_worker_h worker);

extern std::string Tag2String(Tag tag);
