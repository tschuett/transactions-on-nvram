#pragma once

#include <ucp/api/ucp.h>

namespace ucx::rbt {

extern void clientLoop(ucp_ep_h ep, ucp_worker_h ucp_worker,
                       size_t request_size, ucp_context_h ucp_context);

}
