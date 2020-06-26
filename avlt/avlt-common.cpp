#include "avlt-common.h"

#include "avlt-allocator.h"

#include "write-dsl-avlt.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace {

signed height(AllocatorType &Allocator, NodeIndex Root,
              std::set<NodeIndex> &Nodes) {
  if (Root == Nullptr)
    return 0;

  // printf("height: %u %lu\n", Root.getValue(), Nodes.size());
  assert(Nodes.count(Root) == 0);

  Nodes.insert(Root);

  NodeIndex RootLeft =
      LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex RootRight =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();

  signed Left = height(Allocator, RootLeft, Nodes);
  signed Right = height(Allocator, RootRight, Nodes);

  return 1 + std::max(Left, Right);
}

bool assertTreeIntern(AllocatorType &Allocator, NodeIndex Root,
                      std::set<NodeIndex> &Nodes) {
  if (Root == Nullptr)
    return true;

  assert(Nodes.count(Root) == 0);

  Nodes.insert(Root);

  uint64_t RootKey = KeyInNode<NodePtrValue>(Allocator, NodePtrValue(Root))
                         .readValue(); // Allocator.get(Root)->key;
  KeyInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex LeftChild = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root))
                            .readValue(); // Allocator.get(Root)->left;
  NodeIndex RightChild =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root))
          .readValue(); // Allocator.get(Root)->right;

  if (LeftChild != Nullptr) {
    uint64_t LeftChildKey =
        KeyInNode<NodePtrValue>(Allocator, NodePtrValue(LeftChild)).readValue();
    if (not(LeftChildKey < RootKey))
      printf("%u: %" PRIu64 " <? %" PRIu64 "\n", LeftChild.getValue(),
             LeftChildKey, RootKey);
    assert(LeftChildKey < RootKey);
  }

  if (RightChild != Nullptr) {
    uint64_t RightChildKey =
        KeyInNode<NodePtrValue>(Allocator, NodePtrValue(RightChild))
            .readValue();
    if (not(RightChildKey > RootKey)) {
      printf("%u: %" PRIu64 " <? %" PRIu64 "\n", RightChild.getValue(),
             RightChildKey, RootKey);
      Avlt::plotTree(Allocator, Root);
    }
    assert(RightChildKey > RootKey);
  }

  std::set<NodeIndex> LeftNodes = {};
  std::set<NodeIndex> RightNodes = {};
  signed Left = height(Allocator, LeftChild, LeftNodes);
  signed Right = height(Allocator, RightChild, RightNodes);

  // Allocator.get(Root)->Balance;
  int8_t RootBalance =
      BalanceInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  if (RootBalance == -1) {
    if (not(Left > Right))
      Avlt::plotTree(Allocator, Root);

    assert(Left > Right);
  } else if (RootBalance == 0) {
    if (not(Left == Right)) {
      printf("left=%d right=%d: balance=%d root=%u\n", Left, Right, RootBalance,
             Root.getValue());
      Avlt::plotTree(Allocator, Root);
    }
    assert(Left == Right);
  } else if (RootBalance == 1)
    assert(Left < Right);
  else {
    assert(false);
  }

  NodeIndex RootLeft =
      LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex RootRight =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  if (std::abs(Left - Right) > 1) {
    printf("%u %u\n", RootLeft.getValue(), RootRight.getValue());
    printf("%d %d\n", Left, Right);
    assert(false);
  }

  return assertTreeIntern(Allocator, RootLeft, Nodes) and
         assertTreeIntern(Allocator, RootRight, Nodes);
}

unsigned getTreeSizeIntern(AllocatorType &Allocator, NodeIndex Root,
                           std::set<NodeIndex> &Nodes) {
  if (Root == Nullptr)
    return 0;

  assert(Nodes.count(Root) == 0);

  Nodes.insert(Root);

  NodeIndex RootLeft =
      LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex RootRight =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();

  unsigned Left = getTreeSizeIntern(Allocator, RootLeft, Nodes);
  unsigned Right = getTreeSizeIntern(Allocator, RootRight, Nodes);

  return 1 + Left + Right;
}

void plotNode(AllocatorType &Allocator, NodeIndex Root) {
  if (Root == Nullptr)
    return;

  NodeIndex Left =
      LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex Right =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  // NodeIndex Left = Allocator.get(Root)->left;
  // NodeIndex Right = Allocator.get(Root)->right;

  // std::set<NodeIndex> LeftNodes = {};
  // std::set<NodeIndex> RightNodes = {};
  // signed LeftHeight = height(Allocator, Allocator.get(Root)->left,
  // LeftNodes); signed RightHeight = height(Allocator,
  // Allocator.get(Root)->right, RightNodes);

  int8_t RootBalance =
      BalanceInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  uint64_t RootKey =
      KeyInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();

  printf("%u: balance=%d key=%" PRIu64 "\n", Root.getValue(), RootBalance,
         RootKey);
  // printf("  height: %u %u\n", LeftHeight, RightHeight);
  printf("  childs: %u %u\n", Left.getValue(), Right.getValue());
}

void plotTreeIntern(AllocatorType &Allocator, NodeIndex Root,
                    std::set<NodeIndex> &Nodes) {
  if (Root == Nullptr)
    return;

  if (not(Nodes.count(Root) == 0))
    printf("Root: %d\n", Root.getValue());
  assert(Nodes.count(Root) == 0);

  Nodes.insert(Root);

  plotNode(Allocator, Root);

  NodeIndex Left =
      LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex Right =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();

  // NodeIndex Left = Allocator.get(Root)->left;
  // NodeIndex Right = Allocator.get(Root)->right;

  plotTreeIntern(Allocator, Left, Nodes);
  plotTreeIntern(Allocator, Right, Nodes);
}

} // namespace

namespace Avlt {

unsigned getTreeSize(AllocatorType &Allocator, NodeIndex Root) {
  std::set<NodeIndex> Nodes = {};

  if (Root == Nullptr)
    return 0;

  NodeIndex RootLeft =
      LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
  NodeIndex RootRight =
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();

  unsigned Left = getTreeSizeIntern(Allocator, RootLeft, Nodes);
  unsigned Right = getTreeSizeIntern(Allocator, RootRight, Nodes);

  return 1 + Left + Right;
}

bool assertTree(AllocatorType &Allocator, NodeIndex Root) {
  std::set<NodeIndex> Nodes = {};

  printf("run assert\n");
  // plotTree(Allocator, Root);

  return assertTreeIntern(Allocator, Root, Nodes);
}

void plotTree(AllocatorType &Allocator, NodeIndex Root) {
  std::set<NodeIndex> Nodes = {};

  printf("root: %u\n", Root.getValue());

  plotTreeIntern(Allocator, Root, Nodes);
}

unsigned getTreeDepth(AllocatorType &Allocator, NodeIndex root) {
  if (root == Nullptr)
    return 0;

  NodeIndex ln = Allocator.get(root)->left;
  NodeIndex rn = Allocator.get(root)->right;

  unsigned left = getTreeDepth(Allocator, ln);
  unsigned right = getTreeDepth(Allocator, rn);

  return 1 + std::max(left, right);
}

std::optional<unsigned> getLookupDepth(AllocatorType &Allocator, NodeIndex root,
                                       uint64_t key) {
  if (root == Nullptr)
    return std::nullopt;

  uint64_t rootKey = Allocator.get(root)->key;
  if (rootKey == key)
    return 0;

  NodeIndex ln = Allocator.get(root)->left;
  NodeIndex rn = Allocator.get(root)->right;

  if (key < rootKey) {
    std::optional<unsigned> l = getLookupDepth(Allocator, ln, key);
    if (l.has_value())
      return 1 + l.value();
    return l;
  } else if (key > rootKey) {
    std::optional<unsigned> r = getLookupDepth(Allocator, rn, key);
    if (r.has_value())
      return 1 + r.value();
    return r;
  } else {
    assert(false);
  }

  return std::nullopt;
}

} // namespace Avlt
