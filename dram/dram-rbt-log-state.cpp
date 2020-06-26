#include "dram-rbt-log-state.h"

namespace DRAM {
LogState::LogState() { data = RBTLogStructure(); }

void LogState::resetState() { data = RBTLogStructure(); }
} // namespace DRAM
