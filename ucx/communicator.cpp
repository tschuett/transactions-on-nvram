#include "communicator.h"

#include "ucx-config.h"

#include <cassert>
#include <memory>

namespace {
void empty_send_cb(void *request, ucs_status_t status) {}

void empty_recv_cb(void *request, ucs_status_t status,
                   ucp_tag_recv_info_t *info) {}
} // namespace

namespace ucx::config {
std::unique_ptr<Communicator> Comm;
}

namespace ucx {

void Communicator::sendFlushMessage(const CommonMessage &Message) {
  const ucp_tag_t tag_mask = -1;
  ucs_status_ptr_t SendStatus;

  fence();

  SendStatus = ucp_tag_send_nb(ep, &Message, sizeof(CommonMessage),
                               ucp_dt_make_contig(1), 1337, empty_send_cb);

  waitTagSend(worker, SendStatus);

  CommonMessage recvMsg;
  ucs_status_ptr_t RecvStatus =
      ucp_tag_recv_nb(worker, &recvMsg, sizeof(CommonMessage),
                      ucp_dt_make_contig(1), 1337, tag_mask, empty_recv_cb);

  waitTagRecv(worker, RecvStatus);

  assert(recvMsg.tag == Tag::FLUSH_ACK);
}

void Communicator::fence() {
  ucs_status_t Status = ucp_worker_fence(worker);
  assert(Status == UCS_OK);
}

void Communicator::flush() {
  ucs_status_t Status = ucp_worker_flush(worker);
  assert(Status == UCS_OK);
}

} // namespace ucx
