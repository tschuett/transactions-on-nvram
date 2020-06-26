#include "server-avl-loop.h"

#include "passive-avl-dram-allocator.h"
#include "passive-avl-pmdk-allocator.h"
#include "server-avl-assert.h"
#include "server-avl-loop-common.h"
#include "ucx-common.h"

namespace {

void empty_send_cb(void *request, ucs_status_t status) {}
void empty_recv_cb(void *request, ucs_status_t status,
                   ucp_tag_recv_info_t *info) {}

} // namespace

namespace ucx::avlt {

void serverLoop(ucp_ep_h ep, ucp_worker_h worker, size_t request_size,
                ucp_context_h ucp_context) {
  unsigned Count = 0;
  const ucp_tag_t tag_mask = -1;

  // ucx::PassiveAllocator pa = {ucp_context};
  ucx::avlt::PassivePMDKAllocator pa = {ucp_context};

  exchangeKeys(ep, worker, ucp_context, request_size, pa);

  bool done = false;
  while (!done) {
    CommonMessage recvMsg;

    ucs_status_ptr_t RecvStatus =
        ucp_tag_recv_nb(worker, &recvMsg, sizeof(CommonMessage),
                        ucp_dt_make_contig(1), 1337, tag_mask, empty_recv_cb);

    waitTagRecv(worker, RecvStatus);

    switch (recvMsg.tag) {
    case Tag::ASSERT: {
      // printf("assert\n");
      ucx::avlt::assertTree(SimpleAllocator(pa.getNodes()), pa.getRoot());
      CommonMessage SendMsg;
      SendMsg.tag = Tag::ASSERT_ACK;
      ucs_status_ptr_t SendStatus =
          ucp_tag_send_nb(ep, &SendMsg, sizeof(CommonMessage),
                          ucp_dt_make_contig(1), 1337, empty_send_cb);
      waitTagSend(worker, SendStatus);
      // printf("count=%d\n", Count);
      pa.reset();
      Count = 0;
      break;
    }
    case Tag::FLUSH: {
      // printf("flush %u\n", recvMsg.Count);
      Count++;
      pa.flush(recvMsg);
      CommonMessage SendMsg;
      SendMsg.tag = Tag::FLUSH_ACK;
      ucs_status_ptr_t SendStatus =
          ucp_tag_send_nb(ep, &SendMsg, sizeof(CommonMessage),
                          ucp_dt_make_contig(1), 1337, empty_send_cb);
      waitTagSend(worker, SendStatus);
      // printf("count=%d\n", Count);
      // printf("flush done\n");
      break;
    }
    case Tag::FLUSH_ACK:
      assert(false);
      break;
    case Tag::ASSERT_ACK:
      assert(false);
      break;
    case Tag::KEYXCHG:
      assert(false);
      break;
    }
  }
  printf("done\n");
}

} // namespace ucx::avlt
