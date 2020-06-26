#pragma once

#if defined(HAVE_PMDK)

#include "pmdk/pmdk-avl-allocator.h"
#include "pmdk/pmdk-avl-log-state.h"
#include "pmdk/pmdk-avl-sm-state.h"

#elif defined(HAVE_UCX)

#include "active-avl-allocator.h"
#include "active-avl-log.h"
#include "active-avl-sm-state.h"

#else

#include "dram/dram-avl-allocator.h"
#include "dram/dram-avl-log-state.h"
#include "dram/dram-avl-sm-state.h"

#endif
