#include <ucp/api/ucp.h>

#include "server-avl-loop.h"
#include "tree.h"
#include "ucx-common.h"

#include <alloca.h>
#include <netinet/ip.h>

#include <cassert>
#include <cstdio>
#include <cstring>

static void server_accept_cb(ucp_ep_h ep, void *arg) {
  ucx_server_ctx_t *context = static_cast<ucx_server_ctx_t *>(arg);

  /* Save the server's endpoint in the user's context, for future usage */
  context->ep = ep;
}

void doServer(ucp_ep_h ep, ucp_worker_h ucp_worker, size_t request_size,
              ucp_context_h ucp_context) {
  CommonMessage recvMsg;
  CommonMessage sendMsg;
  ucs_status_t recvStatus;
  ucs_status_t sendStatus;

  uint8_t *recvRequest = static_cast<uint8_t *>(alloca(request_size));
  uint8_t *sendRequest = static_cast<uint8_t *>(alloca(request_size));

  recvStatus = ucp_tag_recv_nbr(ucp_worker, &sendMsg, sizeof(sendMsg),
                                ucp_dt_make_contig(1), 1337, 0x0,
                                recvRequest + request_size);
  sendStatus =
      ucp_tag_send_nbr(ep, &sendMsg, sizeof(recvMsg), ucp_dt_make_contig(1),
                       1337, sendRequest + request_size);

  wait(ucp_worker, sendStatus, sendRequest + request_size);
  wait(ucp_worker, recvStatus, recvRequest + request_size);

  ucx::avlt::serverLoop(ep, ucp_worker, request_size, ucp_context);
}

int main(int argc, char **argv) {
  ucp_context_h ucp_context;
  ucp_listener_h listener;
  ucp_worker_h ucp_worker;
  ucp_context_attr_t attr;
  ucp_ep_h ep;
  struct sockaddr_in listen_addr;
  ucp_listener_params_t params;
  ucx_server_ctx_t context;
  ucs_status_t status;

  initUCX(&ucp_context, &ucp_worker);

  attr.field_mask = UCP_ATTR_FIELD_REQUEST_SIZE;
  status = ucp_context_query(ucp_context, &attr);
  assert(status == UCS_OK);
  // assert((attr.field_mask & UCP_ATTR_FIELD_REQUEST_SIZE) ==
  // UCP_ATTR_FIELD_REQUEST_SIZE);

  /* Initialize the server's endpoint to NULL. Once the server's endpoint is
   * created, this field will have a valid value. */
  context.ep = nullptr;

  /* The server will listen on INADDR_ANY */
  memset(&listen_addr, 0, sizeof(struct sockaddr_in));
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_addr.s_addr = INADDR_ANY;
  listen_addr.sin_port = ServerPort;

  params.field_mask = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR |
                      UCP_LISTENER_PARAM_FIELD_ACCEPT_HANDLER;
  params.sockaddr.addr = (const struct sockaddr *)&listen_addr;
  params.sockaddr.addrlen = sizeof(listen_addr);
  params.accept_handler.cb = server_accept_cb;
  params.accept_handler.arg = &context;

  /* Create a listener on the server side to listen on the given address.*/
  status = ucp_listener_create(ucp_worker, &params, &listener);
  if (status == UCS_OK) {
    printf("Waiting for connection...\n");
    /* Wait for the server's callback to set the context->ep field, thus
     * indicating that the server's endpoint was created and is ready to
     * be used */
    while (context.ep == nullptr) {
      ucp_worker_progress(ucp_worker);
    }
  } else {
    fprintf(stderr, "failed to listen (%s)\n", ucs_status_string(status));
  }

  ep = context.ep;

  ucp_ep_print_info(ep, stdout);

  doServer(ep, ucp_worker, attr.request_size, ucp_context);

  ucp_listener_destroy(listener);

  closeEp(ucp_worker, ep);

  ucp_worker_destroy(ucp_worker);

  ucp_cleanup(ucp_context);

  static_assert(sizeof(CommonMessage) < 80);
  static_assert(sizeof(Node) == 32);
};
