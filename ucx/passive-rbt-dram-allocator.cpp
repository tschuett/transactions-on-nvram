#include "passive-rbt-dram-allocator.h"

#include <cassert>
#include <cstring>

namespace {
template <typename T> T *getBaseAddress(ucp_mem_h Handle) {
  ucs_status_t status;
  ucp_mem_attr_t attr;
  attr.field_mask = UCP_MEM_ATTR_FIELD_ADDRESS;
  status = ucp_mem_query(Handle, &attr);
  assert(status == UCS_OK);
  return static_cast<T *>(attr.address);
}

} // namespace
namespace ucx::rbt {

ucp_mem_h PassiveAllocator::getMemHandleForLog() { return MemHandlerForLog; }

ucp_mem_h PassiveAllocator::getMemHandleForSM() { return MemHandlerForSM; }

ucp_mem_h PassiveAllocator::getMemHandleForNodes() {
  return MemHandlerForNodes;
}

ucp_mem_h PassiveAllocator::getMemHandleForCurrentNode() {
  return MemHandlerForCurrentNode;
}

void PassiveAllocator::init() {
  ucs_status_t status;

  // allocate the nodes
  ucp_mem_map_params_t nodesParams;
  nodesParams.field_mask =
      UCP_MEM_MAP_PARAM_FIELD_LENGTH | UCP_MEM_MAP_PARAM_FIELD_FLAGS;
  nodesParams.length = sizeof(Node) * capacity;
  nodesParams.flags = UCP_MEM_MAP_ALLOCATE;
  status = ucp_mem_map(ucp_context, &nodesParams, &MemHandlerForNodes);
  assert(status == UCS_OK);

  NodesP = getBaseAddress<Node>(MemHandlerForNodes);

  void *rkey_buffer_p;
  size_t size_p;
  status =
      ucp_rkey_pack(ucp_context, MemHandlerForNodes, &rkey_buffer_p, &size_p);
  assert(status == UCS_OK);
  assert(size_p == 25);
  ucp_rkey_buffer_release(rkey_buffer_p);

  // allocate the log
  {
    ucp_mem_map_params_t logParams;
    logParams.field_mask =
        UCP_MEM_MAP_PARAM_FIELD_LENGTH | UCP_MEM_MAP_PARAM_FIELD_FLAGS;
    logParams.length = sizeof(RBTLogStructure);
    logParams.flags = UCP_MEM_MAP_ALLOCATE;
    status = ucp_mem_map(ucp_context, &logParams, &MemHandlerForLog);
    assert(status == UCS_OK);

    LogP = getBaseAddress<RBTLogStructure>(MemHandlerForLog);
    assert(((uintptr_t)LogP % 64) == 0);

    memset(LogP, 0, sizeof(RBTLogStructure));

    LogP->Root = Nullptr;
  }

  // allocate the sm
  {
    ucp_mem_map_params_t smParams;
    smParams.field_mask =
        UCP_MEM_MAP_PARAM_FIELD_LENGTH | UCP_MEM_MAP_PARAM_FIELD_FLAGS;
    smParams.length = sizeof(StateStructure);
    smParams.flags = UCP_MEM_MAP_ALLOCATE;
    status = ucp_mem_map(ucp_context, &smParams, &MemHandlerForSM);
    assert(status == UCS_OK);

    SMP = getBaseAddress<StateStructure>(MemHandlerForSM);

    assert(((uintptr_t)SMP % 64) == 0);

    memset(SMP, 0, sizeof(StateStructure));

    SMP->EvenOdd = 0;
    SMP->State = RBTStateKind::C0;
  }

  // current node
  {
    ucp_mem_map_params_t cnParams;
    cnParams.field_mask =
        UCP_MEM_MAP_PARAM_FIELD_LENGTH | UCP_MEM_MAP_PARAM_FIELD_FLAGS;
    cnParams.length = sizeof(uint64_t);
    cnParams.flags = UCP_MEM_MAP_ALLOCATE;
    status = ucp_mem_map(ucp_context, &cnParams, &MemHandlerForCurrentNode);
    assert(status == UCS_OK);

    CurrentNodeP = getBaseAddress<uint64_t>(MemHandlerForCurrentNode);

    assert(((uintptr_t)CurrentNodeP % 64) == 0);

    memset(SMP, 0, sizeof(uint64_t));

    *CurrentNodeP = 1;
  }
}

void PassiveAllocator::flush(CommonMessage Msg) {}

void PassiveAllocator::reset() {
  memset(LogP, 0, sizeof(RBTLogStructure));

  LogP->Root = Nullptr;

  memset(SMP, 0, sizeof(StateStructure));

  SMP->EvenOdd = 0;
  SMP->State = RBTStateKind::C0;

  memset(SMP, 0, sizeof(uint64_t));

  *CurrentNodeP = 1;
}

} // namespace ucx::rbt
