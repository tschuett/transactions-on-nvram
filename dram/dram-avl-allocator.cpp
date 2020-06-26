#include "dram/dram-avl-allocator.h"

#include "avlt-log.h"
#include "dram/dram-avl-log-state.h"
#include "flush.h"
#include "tree.h"

#include <cstdio>

namespace DRAM {

void AVLTAllocator::init() {
  capacity = 1024 * 1024 * 128 + 10; // 10000010;
  data = static_cast<Node *>(malloc(sizeof(Node) * capacity));
  assert(data != nullptr);
}

NodeIndex AVLTAllocator::get() {
  if (current >= capacity)
    printf("%lu %lu\n", current, capacity);
  assert(current < capacity);
  Node *res = new (data + current) Node();

  flushObj(res, FlushKind::NewNode);

  return NodeIndex{current++};
}

NodeIndex AVLTAllocator::getNewNodeFromLog() {
  uint64_t key = log.getLog()->Key;
  uint64_t value = log.getLog()->Value;
  assert(current < capacity);
  Node NewNode = {};
  data[current] = NewNode;
  // Node* res = new (data + current) Node();
  Node *res = data + current;

  res->key = key;
  res->value = value;
  res->Balance = 0;
  res->left = Nullptr;
  res->right = Nullptr;
  res->Up = Nullptr;
  res->UpDir = Direction::Unknown;

  flushObj(res, FlushKind::NewNode);

  return NodeIndex{current++};
}

void AVLTAllocator::resetState() {}

} // namespace DRAM
