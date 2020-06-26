#include <ucp/api/ucp.h>

#include "client-loop-avl.h"
#include "ucx-common.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <string>

namespace {

void empty_send_cb(void *request, ucs_status_t status) {}

void empty_recv_cb(void *request, ucs_status_t status,
                   ucp_tag_recv_info_t *info) {}

void doClient(ucp_ep_h ep, ucp_worker_h ucp_worker, ucp_context_h ucp_context,
              size_t request_size) {
  CommonMessage recvMsg;
  CommonMessage sendMsg;

  ucs_status_ptr_t RecvStatus =
      ucp_tag_recv_nb(ucp_worker, &recvMsg, sizeof(CommonMessage),
                      ucp_dt_make_contig(1), 1337, 0x0, empty_recv_cb);

  ucs_status_ptr_t SendStatus =
      ucp_tag_send_nb(ep, &sendMsg, sizeof(CommonMessage),
                      ucp_dt_make_contig(1), 1337, empty_send_cb);
  waitTagSend(ucp_worker, SendStatus);
  waitTagRecv(ucp_worker, RecvStatus);

  ucx::avlt::clientLoop(ep, ucp_worker, request_size, ucp_context);
}

} // namespace
int main(int argc, char **argv) {
  ucp_ep_params_t ep_params;
  struct sockaddr_in connect_addr;
  ucs_status_t status;
  ucp_context_attr_t attr;

  ucp_context_h ucp_context;
  ucp_worker_h ucp_worker;
  ucp_ep_h ep;

  std::string address_str = {"192.168.0.8"};

  initUCX(&ucp_context, &ucp_worker);

  attr.field_mask = UCP_ATTR_FIELD_REQUEST_SIZE;
  status = ucp_context_query(ucp_context, &attr);
  assert(status == UCS_OK);
  // assert((attr.field_mask & UCP_ATTR_FIELD_REQUEST_SIZE) ==
  // UCP_ATTR_FIELD_REQUEST_SIZE);

  memset(&connect_addr, 0, sizeof(struct sockaddr_in));
  connect_addr.sin_family = AF_INET;
  connect_addr.sin_addr.s_addr = inet_addr(address_str.c_str());
  connect_addr.sin_port = ServerPort;

  /*
   * Endpoint field mask bits:
   * UCP_EP_PARAM_FIELD_FLAGS             - Use the value of the 'flags' field.
   * UCP_EP_PARAM_FIELD_SOCK_ADDR         - Use a remote sockaddr to connect
   *                                        to the remote peer.
   * UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE - Error handling mode - this flag
   *                                        is temporarily required since the
   *                                        endpoint will be closed with
   *                                        UCP_EP_CLOSE_MODE_FORCE which
   *                                        requires this mode.
   *                                        Once UCP_EP_CLOSE_MODE_FORCE is
   *                                        removed, the error handling mode
   *                                        will be removed.
   */
  ep_params.field_mask = UCP_EP_PARAM_FIELD_FLAGS |
                         UCP_EP_PARAM_FIELD_SOCK_ADDR |
                         UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
  ep_params.err_mode = UCP_ERR_HANDLING_MODE_PEER;
  ep_params.flags = UCP_EP_PARAMS_FLAGS_CLIENT_SERVER;
  ep_params.sockaddr.addr = (struct sockaddr *)&connect_addr;
  ep_params.sockaddr.addrlen = sizeof(connect_addr);

  status = ucp_ep_create(ucp_worker, &ep_params, &ep);
  if (status != UCS_OK) {
    fprintf(stderr, "failed to connect to %s (%s)\n", address_str.c_str(),
            ucs_status_string(status));
    assert(false);
  }

  doClient(ep, ucp_worker, ucp_context, attr.request_size);

  closeEp(ucp_worker, ep);

  ucp_worker_destroy(ucp_worker);

  ucp_cleanup(ucp_context);
}
