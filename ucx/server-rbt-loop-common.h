#pragma once

#include <ucp/api/ucp.h>

#include "passive-rbt-dram-allocator.h"
#include "passive-rbt-pmdk-allocator.h"

namespace ucx::rbt {

using Allocator = ucx::rbt::PassivePMDKAllocator;

extern void exchangeKeys(ucp_ep_h ep, ucp_worker_h ucp_worker,
                         ucp_context_h ucp_context, size_t request_size,
                         Allocator &pa);

} // namespace ucx::rbt
