#pragma once

#include <ucp/api/ucp.h>

#include "rbt-allocator-fwd.h"
#include "tree.h"
#include "ucx-config.h"

namespace ucx::rbt {

class ActiveAllocator {
  // size_t capacity = ucx::config::Capacity;
  // size_t current = 1;
  ucp_rkey_h NodesKey = nullptr;
  ucp_rkey_h CurrentNodeKey = nullptr;
  ucp_ep_h ep;
  LogType &log;
  uintptr_t BaseAddressNodes;
  uintptr_t BaseAddressCurrentNode;

public:
  ActiveAllocator(ucp_ep_h ep, LogType &log) : ep(ep), log(log) {}

  ~ActiveAllocator() { ucp_rkey_destroy(NodesKey); }

  void init(ucp_rkey_h NodesKey_, uintptr_t BaseAddressNodes_,
            ucp_rkey_h CurrentNodeKey_, uintptr_t BaseAddressCurrentNode_) {
    NodesKey = NodesKey_;
    BaseAddressNodes = BaseAddressNodes_;
    CurrentNodeKey = CurrentNodeKey_;
    BaseAddressCurrentNode = BaseAddressCurrentNode_;
  }

  NodeIndex getHeadNode() { return NodeIndex(0); }

  void writeColor(NodeIndex Ni, Color c);
  void writeLeft(NodeIndex Ni, NodeIndex Ni2);
  void writeRight(NodeIndex Ni, NodeIndex Ni2);
  void writeNode(NodeIndex Ni, Node Nd);

  void writeKey(NodeIndex Ni, uint64_t Key);
  void writeValue(NodeIndex Ni, uint64_t Key);

  NodeIndex readLeft(NodeIndex Ni);
  NodeIndex readRight(NodeIndex Ni);
  Color readColor(NodeIndex Ni);
  uint64_t readKey(NodeIndex Ni);
  uint64_t readValue(NodeIndex Ni) { assert(false); }

  Node readNode(NodeIndex Ni);

  NodeIndex getNewNodeFromLog();

private:
  uint64_t readCurrent();
  void writeCurrent(uint64_t);
};

} // namespace ucx::rbt
