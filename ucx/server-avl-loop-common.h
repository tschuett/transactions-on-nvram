#pragma once

#include <ucp/api/ucp.h>

#include "passive-avl-dram-allocator.h"
#include "passive-avl-pmdk-allocator.h"

namespace ucx::avlt {

using Allocator = ucx::avlt::PassivePMDKAllocator;

extern void exchangeKeys(ucp_ep_h ep, ucp_worker_h ucp_worker,
                         ucp_context_h ucp_context, size_t request_size,
                         Allocator &pa);

} // namespace ucx::avlt
