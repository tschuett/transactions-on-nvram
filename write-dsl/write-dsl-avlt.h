#pragma once

#include "avlt-allocator-fwd.h"

#ifdef HAVE_UCX

#include "write-dsl-ucx-avlt.h"

#else

#include "write-dsl-local-avlt.h"

#endif
