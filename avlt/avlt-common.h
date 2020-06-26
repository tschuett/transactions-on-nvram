#pragma once

#include "avlt-allocator-fwd.h"
#include "tree.h"

#include <optional>

namespace Avlt {

unsigned getTreeSize(AllocatorType &Allocator, NodeIndex Root);
bool assertTree(AllocatorType &Allocator, NodeIndex Root);
void plotTree(AllocatorType &Allocator, NodeIndex Root);

unsigned getTreeDepth(AllocatorType &Allocator, NodeIndex root);

std::optional<unsigned> getLookupDepth(AllocatorType &Allocator, NodeIndex root,
                                       uint64_t key);

} // namespace Avlt
