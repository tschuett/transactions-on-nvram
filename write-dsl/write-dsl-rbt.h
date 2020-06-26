#pragma once

#include "rbt-allocator-fwd.h"

#ifdef HAVE_UCX

#include "write-dsl-ucx-rbt.h"

#else

#include "write-dsl-local-rbt.h"

#endif
