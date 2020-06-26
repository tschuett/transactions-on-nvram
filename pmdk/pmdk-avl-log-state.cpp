#include "pmdk-avl-log-state.h"

#include <sys/stat.h>

#include <cassert>
#include <string>

#ifdef HAVE_PMDK

#include "flush.h"
#include "statistics.h"

#include <libpmem.h>

namespace PMDK {

AVLTLogState::AVLTLogState() {

  data = nullptr;

  size_t size = sizeof(AVLTLogStructure);

  std::string path = "/mnt/pmem0/user1/avl-log.bin";

  size_t mapped_len = 0;

  data = static_cast<AVLTLogStructure *>(
      pmem_map_file(path.c_str(), size, PMEM_FILE_CREATE, S_IWUSR | S_IRUSR,
                    &mapped_len, nullptr));
  assert(data != nullptr);
  assert(mapped_len == size);
  // assert(is_pmem == 1);
  // assert(pmem_is_pmem(data, size));
}

void AVLTLogState::resetState() {
  new (data) AVLTLogStructure();
  flushObj(data, FlushKind::Allocator);
}

}; // namespace PMDK

#endif
