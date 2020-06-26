#pragma once

#include "rbt-allocator-fwd.h"
#include "tree.h"

extern unsigned countNodes(AllocatorType &Allocator, NodeIndex Root);

extern void plotNode(AllocatorType &allocator, NodeIndex root);

extern void printReachableNodes(AllocatorType &Allocator, NodeIndex Root);
