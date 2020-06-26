#pragma once

#include "avlt-log.h"
#include "tree.h"

#include "dram/dram-avl-log-state.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace DRAM {

class AVLTAllocator {
  AVLTLogState &log;
  Node *data = nullptr;
  std::size_t current = 1;
  std::size_t capacity = 0;

  void init();

public:
  AVLTAllocator(AVLTLogState &Log) : log(Log) { init(); }

  NodeIndex get();
  NodeIndex getNewNodeFromLog();
  Node *get(NodeIndex idx) {
    assert(idx != Nullptr);
    return data + idx.getValue();
  }

  Node readNode(NodeIndex idx) {
    assert(idx != Nullptr);
    return *(data + idx.getValue());
  }

  NodeIndex getHeadNode() { return NodeIndex{0}; }

  std::size_t size() const { return current; }

  void resetState();
};

} // namespace DRAM
