#pragma once

#if defined(HAVE_PMDK)

#include "pmdk-rbt-allocator.h"
#include "pmdk-rbt-log-state.h"
#include "pmdk-rbt-sm-state.h"

#elif defined(HAVE_UCX)

#include "active-rbt-allocator.h"
#include "active-rbt-log.h"
#include "active-rbt-sm-state.h"

#else

#include "dram/dram-rbt-allocator.h"
#include "dram/dram-rbt-log-state.h"
#include "dram/dram-rbt-sm-state.h"

#endif
