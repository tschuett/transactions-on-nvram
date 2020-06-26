#include "dram/dram-rbt-allocator.h"

#include "dram/dram-rbt-log-state.h"
#include "flush.h"
#include "statistics.h"
#include "tree.h"
#include "rbt-log.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <new>

#ifndef HAVE_PMDK

namespace DRAM {
void Allocator::init() {
  capacity = 1024 * 1024 * 128 + 10; // 10000010;
  data = static_cast<Node *>(malloc(sizeof(Node) * capacity));
  assert(data != nullptr);
}

NodeIndex Allocator::get() {
  if (current >= capacity)
    printf("%lu %lu\n", current, capacity);
  assert(current < capacity);
  Node *res = new (data + current) Node();

  flushObj(res, FlushKind::NewNode);

  return NodeIndex{current++};
}

NodeIndex Allocator::getNewNodeFromLog() {
  uint64_t key = log.getLog()->Key;
  uint64_t value = log.getLog()->Value;
  assert(current < capacity);
  Node NewNode = {};
  data[current] = NewNode;
  // Node* res = new (data + current) Node();
  Node *res = data + current;

  res->key = key;
  res->value = value;
  res->left = Nullptr;
  res->right = Nullptr;

  flushObj(res, FlushKind::NewNode);

  return NodeIndex{current++};
}

void Allocator::resetState() {}

} // namespace DRAM

#endif
