#pragma once

#include <cstdint>

enum class WindowKind : uint8_t {
  LogWindow,
  NodeWindow,
  StateWindow,
  CurrentNodeWindow
};
