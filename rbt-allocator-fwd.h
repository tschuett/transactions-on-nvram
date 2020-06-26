#pragma once

#if defined(HAVE_PMDK)

// struct alignas(64) Log;

namespace PMDK {
class Allocator;
class SMState;
class LogState;
} // namespace PMDK

using AllocatorType = PMDK::Allocator;
using SMStateType = PMDK::SMState;
using LogType = PMDK::LogState;

#elif defined(HAVE_UCX)

namespace ucx::rbt {
class ActiveAllocator;
class ActiveLog;
class ActiveSMState;
} // namespace ucx::rbt

using AllocatorType = ucx::rbt::ActiveAllocator;
using SMStateType = ucx::rbt::ActiveSMState;
using LogType = ucx::rbt::ActiveLog;

#else

namespace DRAM {
class Allocator;
class SMState;
class LogState;
} // namespace DRAM

struct alignas(64) LogStructure;

using AllocatorType = DRAM::Allocator;
using SMStateType = DRAM::SMState;
using LogType = DRAM::LogState;

#endif

// struct alignas(64) Log;
