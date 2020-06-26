#include "client-loop-rbt.h"

#include "active-rbt-allocator.h"
#include "active-rbt-log.h"
#include "active-rbt-sm-state.h"
#include "rbt/state-machine-insert.h"
#include "ucx-common.h"
#include "util.h"

#include <sched.h>

#include <chrono>

namespace {

ucp_rkey_h NodesKey = nullptr;
ucp_rkey_h CurrentNodeKey = nullptr;
ucp_rkey_h LogKey = nullptr;
ucp_rkey_h SMKey = nullptr;

void empty_cb(void *request, ucs_status_t status, ucp_tag_recv_info_t *info) {}

void pin() {
  cpu_set_t mask;

  CPU_ZERO(&mask);
  CPU_SET(0, &mask);

  int res = sched_setaffinity(0, sizeof(cpu_set_t), &mask);
  assert(res == 0);
}

void exchangeKeys(ucp_ep_h ep, ucp_worker_h ucp_worker, size_t request_size,
                  ucx::rbt::ActiveAllocator &allocator,
                  ucx::rbt::ActiveSMState &sm, ucx::rbt::ActiveLog &log) {
  const ucp_tag_t tag_mask = -1;
  KeyExchangeMessage recvMsg;
  ucs_status_t Status;
  ucs_status_ptr_t RecvStatus =
      ucp_tag_recv_nb(ucp_worker, &recvMsg, sizeof(KeyExchangeMessage),
                      ucp_dt_make_contig(1), 133755, tag_mask, empty_cb);

  waitTagRecv(ucp_worker, RecvStatus);

  Status = ucp_ep_rkey_unpack(ep, &recvMsg.NodesKey, &NodesKey);
  assert(Status == UCS_OK);
  Status = ucp_ep_rkey_unpack(ep, &recvMsg.SMKey, &SMKey);
  assert(Status == UCS_OK);
  Status = ucp_ep_rkey_unpack(ep, &recvMsg.LogKey, &LogKey);
  assert(Status == UCS_OK);
  Status = ucp_ep_rkey_unpack(ep, &recvMsg.CurrentNodeKey, &CurrentNodeKey);
  assert(Status == UCS_OK);

  allocator.init(NodesKey, recvMsg.BaseAddressNodes, CurrentNodeKey,
                 recvMsg.BaseAddressCurrentNode);
  sm.init(SMKey, recvMsg.BaseAddressSM);
  log.init(LogKey, recvMsg.BaseAddressLog);
}

const uint64_t count = 1000;
const uint64_t iterations = 1000;

void insertServer(ucx::rbt::ActiveAllocator &Allocator,
                  ucx::rbt::ActiveSMState &State, ucx::rbt::ActiveLog &Log,
                  ucp_ep_h ep, ucp_worker_h worker) {
  auto start = std::chrono::high_resolution_clock::now();
  for (uint64_t i = 0; i < count; i++) {
    Rbt::Insert::GoToC0(Allocator, Log, State, i, 512);
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = stop - start;
  printf("insert=%fs\n", diff.count());
  checkConsistency(ep, worker);
}

} // namespace

namespace ucx::rbt {

void clientLoop(ucp_ep_h ep, ucp_worker_h worker, size_t request_size,
                ucp_context_h ucp_context) {
  ActiveLog log = {ep};
  ActiveAllocator allocator = {ep, log};
  ActiveSMState sm = {ep};
  ucx::config::Comm = std::make_unique<Communicator>(ep, worker);

  exchangeKeys(ep, worker, request_size, allocator, sm, log);

  pin();

  printCPU();

  for (unsigned int i = 0; i < iterations; i++) {
    log.reset();
    insertServer(allocator, sm, log, ep, worker);
  }

  static_assert(sizeof(CommonMessage) < 80);

  ucp_rkey_destroy(NodesKey);
  ucp_rkey_destroy(LogKey);
  ucp_rkey_destroy(SMKey);
}

} // namespace ucx::rbt
