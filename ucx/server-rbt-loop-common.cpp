#include "server-rbt-loop-common.h"

#include <cassert>

namespace {

void empty_send_cb(void *request, ucs_status_t status) {}

} // namespace

namespace ucx::rbt {

void exchangeKeys(ucp_ep_h ep, ucp_worker_h ucp_worker,
                  ucp_context_h ucp_context, size_t request_size,
                  Allocator &pa) {
  KeyExchangeMessage sendMsg;
  ucs_status_t status;
  void *rkey_buffer_p = nullptr;
  uint8_t *byte_buffer_p = nullptr;
  size_t size_p;

  ucs_status_ptr_t SendStatus;

  ucp_mem_h LogHandle = pa.getMemHandleForLog();
  ucp_mem_h SMHandle = pa.getMemHandleForSM();
  ucp_mem_h NodesHandle = pa.getMemHandleForNodes();
  ucp_mem_h CurrentNodeHandle = pa.getMemHandleForCurrentNode();

  status = ucp_rkey_pack(ucp_context, LogHandle, &rkey_buffer_p, &size_p);
  assert(status == UCS_OK);
  assert(size_p == 26);
  byte_buffer_p = static_cast<uint8_t *>(rkey_buffer_p);
  for (unsigned i = 0; i < size_p; i++)
    sendMsg.LogKey[i] = byte_buffer_p[i];
  ucp_rkey_buffer_release(rkey_buffer_p);

  status = ucp_rkey_pack(ucp_context, SMHandle, &rkey_buffer_p, &size_p);
  assert(status == UCS_OK);
  assert(size_p == 26);
  byte_buffer_p = static_cast<uint8_t *>(rkey_buffer_p);
  for (unsigned i = 0; i < size_p; i++)
    sendMsg.SMKey[i] = byte_buffer_p[i];
  ucp_rkey_buffer_release(rkey_buffer_p);

  status = ucp_rkey_pack(ucp_context, NodesHandle, &rkey_buffer_p, &size_p);
  assert(status == UCS_OK);
  assert(size_p == 26);
  byte_buffer_p = static_cast<uint8_t *>(rkey_buffer_p);
  for (unsigned i = 0; i < size_p; i++)
    sendMsg.NodesKey[i] = byte_buffer_p[i];
  ucp_rkey_buffer_release(rkey_buffer_p);

  status =
      ucp_rkey_pack(ucp_context, CurrentNodeHandle, &rkey_buffer_p, &size_p);
  assert(status == UCS_OK);
  assert(size_p == 26);
  byte_buffer_p = static_cast<uint8_t *>(rkey_buffer_p);
  for (unsigned i = 0; i < size_p; i++)
    sendMsg.CurrentNodeKey[i] = byte_buffer_p[i];
  ucp_rkey_buffer_release(rkey_buffer_p);

  // base address nodes
  {
    ucp_mem_attr_t attr;
    attr.field_mask = UCP_MEM_ATTR_FIELD_ADDRESS;
    status = ucp_mem_query(NodesHandle, &attr);
    assert(status == UCS_OK);
    sendMsg.BaseAddressNodes = (uintptr_t)attr.address;
  }

  // base address sm
  {
    ucp_mem_attr_t attr;
    attr.field_mask = UCP_MEM_ATTR_FIELD_ADDRESS;
    status = ucp_mem_query(SMHandle, &attr);
    assert(status == UCS_OK);
    sendMsg.BaseAddressSM = (uintptr_t)attr.address;
  }

  // base address log
  {
    ucp_mem_attr_t attr;
    attr.field_mask = UCP_MEM_ATTR_FIELD_ADDRESS;
    status = ucp_mem_query(LogHandle, &attr);
    assert(status == UCS_OK);
    sendMsg.BaseAddressLog = (uintptr_t)attr.address;
  }

  // base address current node
  {
    ucp_mem_attr_t attr;
    attr.field_mask = UCP_MEM_ATTR_FIELD_ADDRESS;
    status = ucp_mem_query(CurrentNodeHandle, &attr);
    assert(status == UCS_OK);
    sendMsg.BaseAddressCurrentNode = (uintptr_t)attr.address;
  }

  sendMsg.tag = Tag::KEYXCHG;
  SendStatus = ucp_tag_send_nb(ep, &sendMsg, sizeof(KeyExchangeMessage),
                               ucp_dt_make_contig(1), 133755, empty_send_cb);

  waitTagSend(ucp_worker, SendStatus);
}

} // namespace ucx::rbt
