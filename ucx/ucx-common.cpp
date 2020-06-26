#include "ucx-common.h"

#include "util.h"

#include <ucp/api/ucp.h>

#include <cassert>
#include <cstring>

namespace {
void empty_send_cb(void *request, ucs_status_t status) {}

void empty_recv_cb(void *request, ucs_status_t status,
                   ucp_tag_recv_info_t *info) {}
} // namespace

void closeEp(ucp_worker_h ucp_worker, ucp_ep_h ep) {
  ucs_status_t status;
  void *close_req;

  close_req = ucp_ep_close_nb(ep, UCP_EP_CLOSE_MODE_FORCE);
  if (UCS_PTR_IS_PTR(close_req)) {
    do {
      ucp_worker_progress(ucp_worker);
      status = ucp_request_check_status(close_req);
    } while (status == UCS_INPROGRESS);

    ucp_request_free(close_req);
  } else if (UCS_PTR_STATUS(close_req) != UCS_OK) {
    fprintf(stderr, "failed to close ep %p\n", (void *)ep);
  }
}

void initUCX(ucp_context_h *ucp_context, ucp_worker_h *ucp_worker) {
  ucp_worker_params_t worker_params;
  ucp_params_t ucp_params;
  ucp_config_t *config;
  ucs_status_t status;

  memset(&ucp_params, 0, sizeof(ucp_params));
  memset(&worker_params, 0, sizeof(worker_params));

  /* UCP initialization */
  status = ucp_config_read(NULL, NULL, &config);
  if (status != UCS_OK) {
    fprintf(stderr, "failed to ucp_config_read (%s)\n",
            ucs_status_string(status));
    assert(false);
  }

  ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
  ucp_params.features =
      UCP_FEATURE_TAG | UCP_FEATURE_RMA; // | UCP_FEATURE_AMO64;

  status = ucp_init(&ucp_params, config, ucp_context);
  ucp_config_release(config);
  if (status != UCS_OK) {
    fprintf(stderr, "failed to ucp_init (%s)\n", ucs_status_string(status));
    assert(false);
  }

  worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
  worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;

  status = ucp_worker_create(*ucp_context, &worker_params, ucp_worker);
  if (status != UCS_OK) {
    fprintf(stderr, "failed to ucp_worker_create (%s)\n",
            ucs_status_string(status));
    assert(false);
  }
}

void wait(ucp_worker_h ucp_worker, ucs_status_t status, uint8_t *request) {
  ucs_status_t Status = status;
  if (Status == UCS_INPROGRESS) {
    do {
      ucp_worker_progress(ucp_worker);
      Status = ucp_request_check_status(request);
    } while (Status == UCS_INPROGRESS);
  }
  if (Status != UCS_OK)
    printf("failed to wait (%s)\n", ucs_status_string(Status));
  assert(Status == UCS_OK);
}

void waitTagRecv(ucp_worker_h Worker, ucs_status_ptr_t Request) {
  ucp_tag_recv_info_t info;
  ucs_status_t Status = ucp_tag_recv_request_test(Request, &info);
  while (Status == UCS_INPROGRESS) {
    ucp_worker_progress(Worker);
    Status = ucp_tag_recv_request_test(Request, &info);
  }
  if (Status != UCS_OK) {
    fprintf(stderr, "failed to ucp_tag_recv_request_test (%s)\n",
            ucs_status_string(Status));
    printStackTrace();
    assert(false);
  }
  assert(Status == UCS_OK);
  ucp_request_free(Request);
}

void waitTagSend(ucp_worker_h ucp_worker, ucs_status_ptr_t Request) {
  if (UCS_PTR_STATUS(Request) == UCS_OK)
    return;

  ucs_status_t Status = ucp_request_check_status(Request);
  while (Status == UCS_INPROGRESS) {
    ucp_worker_progress(ucp_worker);
    Status = ucp_request_check_status(Request);
  }
  assert(Status == UCS_OK);
  ucp_request_free(Request);
}

  void checkConsistency(ucp_ep_h ep, ucp_worker_h worker) {
  CommonMessage recvMsg;
  CommonMessage sendMsg;
  ucp_tag_message_h msg_tag = nullptr;
  const ucp_tag_t tag = 1337;
  const ucp_tag_t tag_mask = -1;
  ucp_tag_recv_info_t info_tag;

  sendMsg.tag = Tag::ASSERT;
  ucs_status_ptr_t SendStatus =
      ucp_tag_send_nb(ep, &sendMsg, sizeof(CommonMessage),
                      ucp_dt_make_contig(1), tag, empty_send_cb);

  waitTagSend(worker, SendStatus);

  for (;;) {
    msg_tag = ucp_tag_probe_nb(worker, tag, tag_mask, 1, &info_tag);
    if (msg_tag != nullptr) {
      /* Message arrived */
      break;
    }
    ucp_worker_progress(worker);
  }
  assert(info_tag.length == sizeof(CommonMessage));

  ucs_status_ptr_t request =
      ucp_tag_msg_recv_nb(worker, &recvMsg, sizeof(CommonMessage),
                          ucp_dt_make_contig(1), msg_tag, empty_recv_cb);

  if (UCS_PTR_IS_ERR(request)) {
    fprintf(stderr, "unable to receive UCX data message (%u)\n",
            UCS_PTR_STATUS(request));
    assert(false);
  } else {
    waitTagRecv(worker, request);
    // request->completed = 0;
    //ucp_request_release(request);
    // printf("UCX data message was received\n");
  }

  if (recvMsg.tag != Tag::ASSERT_ACK)
    printf("%s\n", Tag2String(recvMsg.tag).c_str());
  assert(recvMsg.tag == Tag::ASSERT_ACK);
};

std::string Tag2String(Tag tag) {
  switch (tag) {
  case Tag::FLUSH:
    return "FLUSH";
  case Tag::FLUSH_ACK:
    return "FLUSH_ACK";
  case Tag::ASSERT:
    return "ASSERT";
  case Tag::ASSERT_ACK:
    return "ASSERT_ACK";
  case Tag::KEYXCHG:
    return "KEYXCHG";
  }
}
