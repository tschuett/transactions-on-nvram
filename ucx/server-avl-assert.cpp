#include "server-avl-assert.h"

#include <cstdio>

namespace {

// bool isRed(ucx::rbt::SimpleAllocator &allocator, NodeIndex root) {
//  return root != Nullptr && allocator.get(root)->color == Color::Red;
//}
//
// int rb_assert(ucx::rbt::SimpleAllocator allocator, NodeIndex root) {
//  int lh, rh;
//
//  if (root == Nullptr) {
//    return 1;
//  } else {
//    NodeIndex ln = allocator.get(root)->left;
//    NodeIndex rn = allocator.get(root)->right;
//
//    /* Consecutive red links */
//    if (isRed(allocator, root)) {
//      if (isRed(allocator, ln) || isRed(allocator, rn)) {
//        printf("Red violation\n");
//        return 0;
//      }
//    }
//
//    lh = rb_assert(allocator, ln);
//    rh = rb_assert(allocator, rn);
//
//    /* Invalid binary search tree */
//    if ((ln != Nullptr && allocator.get(ln)->key >= allocator.get(root)->key)
//    ||
//        (rn != Nullptr && allocator.get(rn)->key <= allocator.get(root)->key))
//        {
//      printf("Binary tree violation\n");
//      return 0;
//    }
//
//    /* Black height mismatch */
//    if (lh != 0 && rh != 0 && lh != rh) {
//      return 0;
//    }
//
//    /* Only count black links */
//    if (lh != 0 && rh != 0) {
//      return isRed(allocator, root) ? lh : lh + 1;
//    } else {
//      return 0;
//    }
//  }
//}

} // namespace

namespace ucx::avlt {

void assertTree(SimpleAllocator sa, NodeIndex Root) {
  // printf("root=%d\n", Root.getValue());
  ///*int res =*/rb_assert(sa, Root);
  // printf("depth=%d\n", res);
}

} // namespace ucx::avlt
