#include "pmdk-rbt-allocator.h"

#include "flush.h"
#include "rbt-log.h"

#include <sys/stat.h>

#include <cassert>
#include <cstdio>
#include <iosfwd>
#include <new>
#include <numeric>
#include <string>

#ifdef HAVE_PMDK

#include <libpmem.h>

namespace PMDK {
void Allocator::init() {

  capacity = 10000010;
  size_t size = capacity * sizeof(Node);
  // data = static_cast<Node*>(malloc(sizeof(Node) * capacity));

  std::string NodesPath = "/mnt/pmem0/user1/nodes.bin";
  std::string CurrentPath = "/mnt/pmem0/user1/current.bin";

  size_t mapped_len = 0;

  data = static_cast<Node *>(pmem_map_file(NodesPath.c_str(), size,
                                           PMEM_FILE_CREATE, S_IWUSR | S_IRUSR,
                                           &mapped_len, nullptr));
  assert(data != nullptr);
  assert(mapped_len == size);
  // assert(is_pmem == 1);
  // assert(pmem_is_pmem(data, size));

  current = static_cast<uint64_t *>(
      pmem_map_file(CurrentPath.c_str(), sizeof(uint64_t), PMEM_FILE_CREATE,
                    S_IWUSR | S_IRUSR, &mapped_len, nullptr));
  assert(current != nullptr);
  assert(mapped_len == sizeof(uint64_t));
  // assert(is_pmem == 1);
  // assert(pmem_is_pmem(data, size));
}

void Allocator::resetState() {
  Node *res = new (data + 0) Node();
  res->left = Nullptr;
  res->right = Nullptr;

  flushObj(res, FlushKind::Allocator);

  *current = 1;
  flushObj(current, FlushKind::Allocator);
}

NodeIndex Allocator::get() {
  if (*current >= capacity)
    printf("%lu %lu\n", *current, capacity);
  assert(*current < capacity);
  Node *res = new (data + *current) Node();

  flushObj(res, FlushKind::Allocator);

  uint64_t oldCurrent = *current;

  *current = *current + 1;
  flushObj(current, FlushKind::Allocator);

  return NodeIndex{oldCurrent};
}

NodeIndex Allocator::getNewNodeFromLog() {
  uint64_t key = log.getLog()->Key;
  uint64_t value = log.getLog()->Value;
  assert(*current < capacity);
  Node *res = new (data + *current) Node();

  res->key = key;
  res->value = value;
  res->left = Nullptr;
  res->right = Nullptr;

  flushObj(res, FlushKind::Allocator);

  uint64_t oldCurrent = *current;

  *current = *current + 1;
  flushObj(current, FlushKind::Allocator);

  return NodeIndex{oldCurrent};
}
} // namespace PMDK

#endif
