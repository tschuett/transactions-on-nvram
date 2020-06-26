#pragma once

#include "rbt-allocator-fwd.h"
#include "tree.h"

#include <optional>

extern int rb_assert(AllocatorType &allocator, LogType &log, NodeIndex root);

extern unsigned getTreeSize(AllocatorType &Allocator, NodeIndex root);

extern unsigned getTreeDepth(AllocatorType &Allocator, NodeIndex root);

extern std::optional<unsigned> getLookupDepth(AllocatorType &Allocator,
                                              NodeIndex root, uint64_t key);
