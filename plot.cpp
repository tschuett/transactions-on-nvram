#include "plot.h"

#include "rbt-allocator.h"
#include "tree.h"
#include "write-dsl-rbt.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace {
void collectReachableNodes(AllocatorType &Allocator, NodeIndex Root,
                           std::set<NodeIndex> &Nodes) {
  if (Root == Nullptr)
    return;

  if (Nodes.count(Root) == 1) {
    printf("has cycle\n");
    return;
  }

  Nodes.insert(Root);

  NodeIndex ln =
      LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex rn =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();

  collectReachableNodes(Allocator, ln, Nodes);
  collectReachableNodes(Allocator, rn, Nodes);
}

unsigned countNodes(AllocatorType &Allocator, NodeIndex Root,
                    std::set<NodeIndex> &Nodes) {
  if (Root == Nullptr)
    return 0;

  if (Nodes.count(Root) == 1) {
    printf("has cycle\n");
    return 0;
  }

  Nodes.insert(Root);

  NodeIndex ln =
      LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex rn =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();

  return 1 + countNodes(Allocator, ln, Nodes) +
         countNodes(Allocator, rn, Nodes);
}

} // namespace

unsigned countNodes(AllocatorType &Allocator, NodeIndex Root) {
  std::set<NodeIndex> Nodes;

  return countNodes(Allocator, Root, Nodes);
}

void plotNode(AllocatorType &Allocator, NodeIndex Root) {
  if (Root == Nullptr)
    return;

  NodeIndex ln =
      LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex rn =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  uint64_t RootKey =
      KeyInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  Color RootColor =
      ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();

  // Node *node = allocator.get(root);
#ifdef __APPLE__
  printf("idx=%u color=%s key=%llu", Root.getValue(),
         Color2String(RootColor).c_str(), RootKey);
#else
  printf("idx=%u color=%s key=%lu", Root.getValue(),
         Color2String(RootColor).c_str(), RootKey);
#endif
  if (ln != Nullptr)
    printf(" left=%u", ln.getValue());
  else
    printf(" left=Nullptr");

  if (rn != Nullptr)
    printf(" right=%u\n", rn.getValue());
  else
    printf(" right=Nullptr\n");
}

void printReachableNodes(AllocatorType &Allocator, NodeIndex Root) {
  std::set<NodeIndex> Nodes;
  std::vector<NodeIndex> NodesVector;
  collectReachableNodes(Allocator, Root, Nodes);
  std::copy(Nodes.begin(), Nodes.end(), std::back_inserter(NodesVector));
  std::sort(NodesVector.begin(), NodesVector.end());

  for (NodeIndex N : NodesVector)
    printf("%u ", N.getValue());
  printf("\n");

  for (NodeIndex N : NodesVector)
    plotNode(Allocator, N);
}
