#include "jsw-common.h"

#include "plot.h"
#include "rbt-allocator.h"
#include "tree.h"

#include <cstdio>
#include <optional>
#include <set>

namespace {
bool isRed(AllocatorType &allocator, NodeIndex root) {
  return root != Nullptr && allocator.get(root)->color == Color::Red;
}

int rb_assert_intern(AllocatorType &allocator, LogType &log, NodeIndex root,
                     std::set<NodeIndex> &VisitedNodes) {
  int lh, rh;

  if (root == Nullptr) {
    return 1;
  } else {

    // if (VisitedNodes.find(root) != VisitedNodes.end()) {
    //  printf("%lu\n", root.getValue());
    //}

    assert(VisitedNodes.find(root) == VisitedNodes.end());

    VisitedNodes.insert(root);

    NodeIndex ln = allocator.get(root)->left;
    NodeIndex rn = allocator.get(root)->right;

    assert(ln != root);
    if (rn == root)
      printf("%u\n", root.getValue());
    assert(rn != root);

    /* Consecutive red links */
    if (isRed(allocator, root)) {
      if (isRed(allocator, ln) || isRed(allocator, rn)) {
        printf("Red violation\n");
        if (isRed(allocator, ln))
          printf("root=%u ln=%u\n", root.getValue(), ln.getValue());
        else
          printf("root=%u rn=%u\n", root.getValue(), rn.getValue());
        exit(EXIT_FAILURE);
        return 0;
      }
    }

    lh = rb_assert_intern(allocator, log, ln, VisitedNodes);
    rh = rb_assert_intern(allocator, log, rn, VisitedNodes);

    /* Invalid binary search tree */
    if ((ln != Nullptr && allocator.get(ln)->key >= allocator.get(root)->key) ||
        (rn != Nullptr && allocator.get(rn)->key <= allocator.get(root)->key)) {
      printf("Binary tree violation\n");
      assert(false);
      return 0;
    }

    /* Black height mismatch */
    if (lh != 0 && rh != 0 && lh != rh) {
      printf("Black violation\n");
      assert(false);
      return 0;
    }

    /* Only count black links */
    if (lh != 0 && rh != 0) {
      return isRed(allocator, root) ? lh : lh + 1;
    } else {
      return 0;
    }
  }
}

} // namespace

int rb_assert(AllocatorType &Allocator, LogType &log, NodeIndex root) {
  std::set<NodeIndex> VisitedNodes;

  return rb_assert_intern(Allocator, log, root, VisitedNodes);
}

unsigned getTreeSize(AllocatorType &Allocator, NodeIndex root) {
  if (root == Nullptr)
    return 0;

  NodeIndex ln = Allocator.get(root)->left;
  NodeIndex rn = Allocator.get(root)->right;

  return 1 + getTreeSize(Allocator, ln) + getTreeSize(Allocator, rn);
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
