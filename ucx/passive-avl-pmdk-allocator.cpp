#include "passive-avl-pmdk-allocator.h"

#include "flush.h"

#include <libpmem.h>
#include <sys/stat.h>

#include <cassert>
#include <cstring>

namespace ucx::avlt {

ucp_mem_h PassivePMDKAllocator::getMemHandleForLog() {
  return MemHandlerForLog;
}

ucp_mem_h PassivePMDKAllocator::getMemHandleForSM() { return MemHandlerForSM; }

ucp_mem_h PassivePMDKAllocator::getMemHandleForNodes() {
  return MemHandlerForNodes;
}

ucp_mem_h PassivePMDKAllocator::getMemHandleForCurrentNode() {
  return MemHandlerForCurrentNode;
}

void PassivePMDKAllocator::init() {
  initPMDK();
  registerMemory();
}

void PassivePMDKAllocator::initPMDK() {
  // open nodes file
  {
    std::string path = "/mnt/pmem0/user1/ucx/nodes.bin";
    size_t size = sizeof(Node) * capacity;
    size_t mapped_len = 0;

    NodesP = static_cast<Node *>(
        pmem_map_file(path.c_str(), size, PMEM_FILE_CREATE, S_IWUSR | S_IRUSR,
                      &mapped_len, nullptr));
    assert(NodesP != nullptr);
    assert(mapped_len == size);
  }

  // open log structure file
  {
    std::string path = "/mnt/pmem0/user1/ucx/log.bin";
    size_t size = sizeof(AVLTLogStructure);
    size_t mapped_len = 0;

    LogP = static_cast<AVLTLogStructure *>(
        pmem_map_file(path.c_str(), size, PMEM_FILE_CREATE, S_IWUSR | S_IRUSR,
                      &mapped_len, nullptr));
    assert(LogP != nullptr);
    assert(mapped_len == size);
  }

  // open state structure file
  {
    std::string path = "/mnt/pmem0/user1/ucx/state.bin";
    size_t size = sizeof(StateStructure);
    size_t mapped_len = 0;

    SMP = static_cast<StateStructure *>(
        pmem_map_file(path.c_str(), size, PMEM_FILE_CREATE, S_IWUSR | S_IRUSR,
                      &mapped_len, nullptr));
    assert(SMP != nullptr);
    assert(mapped_len == size);
  }

  // open current node file
  {
    std::string path = "/mnt/pmem0/user1/ucx/current.bin";
    size_t size = sizeof(uint64_t);
    size_t mapped_len = 0;

    CurrentNodeP = static_cast<uint64_t *>(
        pmem_map_file(path.c_str(), size, PMEM_FILE_CREATE, S_IWUSR | S_IRUSR,
                      &mapped_len, nullptr));
    assert(CurrentNodeP != nullptr);
    assert(mapped_len == size);
  }
}

void PassivePMDKAllocator::registerMemory() {
  ucs_status_t status;

  // register the nodes
  {
    ucp_mem_map_params_t nodesParams;
    nodesParams.field_mask =
        UCP_MEM_MAP_PARAM_FIELD_LENGTH | UCP_MEM_MAP_PARAM_FIELD_ADDRESS;
    nodesParams.length = sizeof(Node) * capacity;
    nodesParams.address = NodesP;
    status = ucp_mem_map(ucp_context, &nodesParams, &MemHandlerForNodes);
    assert(status == UCS_OK);
  }

  {
    void *rkey_buffer_p;
    size_t size_p;
    status =
        ucp_rkey_pack(ucp_context, MemHandlerForNodes, &rkey_buffer_p, &size_p);
    assert(status == UCS_OK);
    assert(size_p == 26);
    ucp_rkey_buffer_release(rkey_buffer_p);
  }

  // register the log
  {
    ucp_mem_map_params_t logParams;
    logParams.field_mask =
        UCP_MEM_MAP_PARAM_FIELD_LENGTH | UCP_MEM_MAP_PARAM_FIELD_ADDRESS;
    logParams.length = sizeof(AVLTLogStructure);
    logParams.address = LogP;
    status = ucp_mem_map(ucp_context, &logParams, &MemHandlerForLog);
    assert(status == UCS_OK);

    assert(((uintptr_t)LogP % 64) == 0);

    memset(LogP, 0, sizeof(AVLTLogStructure));

    LogP->Root = Nullptr;
  }

  // register the sm
  {
    ucp_mem_map_params_t smParams;
    smParams.field_mask =
        UCP_MEM_MAP_PARAM_FIELD_LENGTH | UCP_MEM_MAP_PARAM_FIELD_ADDRESS;
    smParams.length = sizeof(StateStructure);
    smParams.address = SMP;
    status = ucp_mem_map(ucp_context, &smParams, &MemHandlerForSM);
    assert(status == UCS_OK);

    assert(((uintptr_t)SMP % 64) == 0);

    memset(SMP, 0, sizeof(StateStructure));

    SMP->EvenOdd = 0;
    SMP->State = RBTStateKind::C0;
  }

  // register the current node
  {
    ucp_mem_map_params_t cnParams;
    cnParams.field_mask =
        UCP_MEM_MAP_PARAM_FIELD_LENGTH | UCP_MEM_MAP_PARAM_FIELD_ADDRESS;
    cnParams.length = sizeof(uint64_t);
    cnParams.address = CurrentNodeP;
    status = ucp_mem_map(ucp_context, &cnParams, &MemHandlerForCurrentNode);
    assert(status == UCS_OK);

    assert(((uintptr_t)CurrentNodeP % 64) == 0);

    memset(CurrentNodeP, 0, sizeof(uint64_t));

    *CurrentNodeP = 1;
  }
}

void PassivePMDKAllocator::flush(CommonMessage Msg) {
  // printf("%u %u %u\n", Msg.Flush[i].Offset, Msg.Flush[i].Size, Msg.Count);
  for (unsigned i = 0; i < Msg.Count; i++) {
    switch (Msg.Flush[i].Window) {
    case WindowKind::LogWindow: {
      flushRange((uint8_t *)LogP + Msg.Flush[i].Offset, Msg.Flush[i].Size,
                 FlushKind::Misc);
      break;
    }
    case WindowKind::NodeWindow: {
      // printf("Node\n");
      flushRange((uint8_t *)NodesP + Msg.Flush[i].Offset, Msg.Flush[i].Size,
                 FlushKind::Misc);
      break;
    }
    case WindowKind::StateWindow: {
      // printf("State\n");
      flushRange((uint8_t *)SMP + Msg.Flush[i].Offset, Msg.Flush[i].Size,
                 FlushKind::Misc);
      break;
    }
    case WindowKind::CurrentNodeWindow: {
      // printf("CurrentNode\n");
      flushRange((uint8_t *)CurrentNodeP + Msg.Flush[i].Offset,
                 Msg.Flush[i].Size, FlushKind::Misc);
      break;
    }
    }
  }
}

void PassivePMDKAllocator::reset() {
  memset(LogP, 0, sizeof(AVLTLogStructure));

  LogP->Root = Nullptr;

  memset(SMP, 0, sizeof(StateStructure));

  SMP->EvenOdd = 0;
  SMP->State = RBTStateKind::C0;

  memset(SMP, 0, sizeof(uint64_t));

  *CurrentNodeP = 1;
}

} // namespace ucx::avlt
