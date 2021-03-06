#pragma once

#include <ucp/api/ucp.h>

#include "rbt-log.h"
#include "state-kind.h"
#include "tree.h"
#include "ucx-config.h"

namespace ucx::rbt {
class PassivePMDKAllocator {
  size_t capacity = ucx::config::Capacity;
  Node *NodesP = nullptr;
  RBTLogStructure *LogP = nullptr;
  StateStructure *SMP = nullptr;
  uint64_t *CurrentNodeP = nullptr;
  ucp_mem_h MemHandlerForLog = nullptr;
  ucp_mem_h MemHandlerForSM = nullptr;
  ucp_mem_h MemHandlerForNodes = nullptr;
  ucp_mem_h MemHandlerForCurrentNode = nullptr;
  ucp_context_h ucp_context;

public:
  PassivePMDKAllocator(ucp_context_h ucp_context) : ucp_context(ucp_context) {
    init();
  }

  ucp_mem_h getMemHandleForLog();
  ucp_mem_h getMemHandleForSM();
  ucp_mem_h getMemHandleForNodes();
  ucp_mem_h getMemHandleForCurrentNode();

  NodeIndex getRoot() const { return LogP->Root; };
  Node *getNodes() const { return NodesP; }

  void flush(CommonMessage Msg);

  void reset();

private:
  void initPMDK();
  void registerMemory();
  void init();
};
} // namespace ucx::rbt
