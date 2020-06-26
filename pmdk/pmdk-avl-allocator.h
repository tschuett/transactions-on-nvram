#pragma once

#ifdef HAVE_PMDK

#include "pmdk-avl-log-state.h"
#include "tree.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace PMDK {

class AVLTAllocator {
  PMDK::AVLTLogState &log;
  Node *data = nullptr;
  uint64_t *current = nullptr;
  std::size_t capacity = 0;

  void init();

public:
  AVLTAllocator(PMDK::AVLTLogState &log) : log(log) { init(); };

  NodeIndex get();
  NodeIndex getNewNodeFromLog();
  Node *get(NodeIndex idx) {
    assert(idx != Nullptr);
    return data + idx.getValue();
  }
  NodeIndex getHeadNode() { return NodeIndex{0}; }

  Node readNode(NodeIndex idx) {
    assert(idx != Nullptr);
    return *(data + idx.getValue());
  }

  std::size_t size() const { return *current; }

  void resetState();
};
} // namespace PMDK

#endif
