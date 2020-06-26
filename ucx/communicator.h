#pragma once

#include "ucx-common.h"

#include <ucp/api/ucp.h>

namespace ucx {
class Communicator {
  ucp_ep_h ep;
  ucp_worker_h worker;

public:
  Communicator(ucp_ep_h ep, ucp_worker_h worker) : ep(ep), worker(worker) {}

  void sendFlushMessage(const CommonMessage& Message);
  void fence();
  void flush();
};
} // namespace ucx
