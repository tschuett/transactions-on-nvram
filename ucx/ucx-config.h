#pragma once

#include "communicator.h"

#include <memory>

namespace ucx::config {

  const size_t Capacity = 10000010;
  const bool CacheLog = true;
  const bool AsyncPut = true;
  const bool Flush = false;

  extern std::unique_ptr<Communicator> Comm;
}
