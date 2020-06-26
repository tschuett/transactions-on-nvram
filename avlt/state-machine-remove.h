#pragma once

#include "avlt-allocator-fwd.h"

#include <cstdint>

namespace Avlt::Remove {

extern void GoToC0(AllocatorType &Allocator, LogType &log, SMStateType &State,
                   uint64_t Key);

extern void RecoverToC0(AllocatorType &Allocator, LogType &log,
                        SMStateType &State);

} // namespace Avlt::Remove
