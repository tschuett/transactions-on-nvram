#pragma once

#if defined(HAVE_PMDK)

// struct alignas(64) Log;

namespace PMDK {
class AVLTAllocator;
class AVLTSMState;
class AVLTLogState;
} // namespace PMDK

using AllocatorType = PMDK::AVLTAllocator;
using SMStateType = PMDK::AVLTSMState;
using LogType = PMDK::AVLTLogState;

#elif defined(HAVE_UCX)

namespace ucx::avlt {
class ActiveAllocator;
class ActiveLog;
class ActiveSMState;
} // namespace ucx::avlt

using AllocatorType = ucx::avlt::ActiveAllocator;
using SMStateType = ucx::avlt::ActiveSMState;
using LogType = ucx::avlt::ActiveLog;

#else

namespace DRAM {
class AVLTAllocator;
class AVLTSMState;
class AVLTLogState;
} // namespace DRAM

struct alignas(64) LogStructure;

using AllocatorType = DRAM::AVLTAllocator;
using SMStateType = DRAM::AVLTSMState;
using LogType = DRAM::AVLTLogState;

#endif

// struct alignas(64) Log;
