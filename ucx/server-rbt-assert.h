#pragma once

#include "tree.h"

namespace ucx::rbt {

class SimpleAllocator {
  Node *Nodes = nullptr;

public:
  SimpleAllocator(Node *Nodes) : Nodes(Nodes) {}

  Node *get(NodeIndex Ni) { return &Nodes[Ni.getValue()]; }
};

extern void assertTree(SimpleAllocator sa, NodeIndex Root);

} // namespace ucx::rbt
