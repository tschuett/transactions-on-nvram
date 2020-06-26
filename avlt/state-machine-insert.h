#pragma once

#include "avlt-allocator-fwd.h"

#include <cstdint>

namespace Avlt::Insert {

extern void GoToC0(AllocatorType &Allocator, LogType &log, SMStateType &State,
                   uint64_t Key, uint64_t Value);

extern void RecoverToC0(AllocatorType &Allocator, LogType &log,
                        SMStateType &State);

} // namespace avlt::Insert
