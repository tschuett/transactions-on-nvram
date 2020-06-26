#include "avlt-allocator.h"
#include "avlt-log.h"
#include "avlt/avlt-common.h"
#include "avlt/state-machine-insert.h"
#include "avlt/state-machine-remove.h"
#include "state-kind.h"
#include "statistics.h"
#include "tree.h"
#include "util.h"

#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <optional>
#include <random>
#include <cinttypes>

unsigned Count = 10000000; // 1024 * 1024; // + 1024;
unsigned Seed = 0;

namespace {
#ifdef __linux__
void pin() {
  cpu_set_t mask;

  CPU_ZERO(&mask);
  CPU_SET(0, &mask);

  int res = sched_setaffinity(0, sizeof(cpu_set_t), &mask);
  assert(res == 0);
  res += 1;
}
#endif

} // namespace

static void runInsertRemove(AllocatorType &Allocator, SMStateType &State,
                            LogType &Log) {

  std::random_device rd;
  std::mt19937 g(rd());

  g.seed(Seed);

  std::vector<int> v;
  v.reserve(Count);
  for (unsigned int i = 0; i < Count; i++)
    v.push_back(i);

  std::shuffle(v.begin(), v.end(), g);

  auto start = std::chrono::high_resolution_clock::now();
  for (unsigned i = 0; i < Count; i++) {
    Avlt::Insert::GoToC0(Allocator, Log, State, v[i], 512);
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = stop - start;
  printf("insert=%fs\n", diff.count());

  // shuffle for remove
  std::shuffle(v.begin(), v.end(), g);

  start = std::chrono::high_resolution_clock::now();
  for (unsigned i = 0; i < Count; i++) {
    Avlt::Remove::GoToC0(Allocator, Log, State, v[i]);
  }
  stop = std::chrono::high_resolution_clock::now();
  diff = stop - start;
  printf("remove=%fs\n", diff.count());
}

static void recoverInsert(AllocatorType &Allocator, SMStateType &State,
                          LogType &Log) {
#ifdef __APPLE__
  printf("recover in %s; key=%llu\n",
         StateKind2String(State.getStateKind()).c_str(), Log.getLog()->Key);
#else
  printf("recover in %s; key=%lu\n",
         StateKind2String(State.getStateKind()).c_str(), Log.getLog()->Key);
#endif
  Avlt::Insert::RecoverToC0(Allocator, Log, State);
  assert(Avlt::assertTree(Allocator, Log.getLog()->Root));
}

static void runInsertWithStatistics(AllocatorType &Allocator,
                                    SMStateType &State, LogType &Log) {
  // std::fstream f("flushes.txt", f.out | f.trunc);

  std::random_device rd;
  std::mt19937 g(rd());

  g.seed(Seed);

  std::vector<int> v;
  for (unsigned int i = 0; i < Count; i++)
    v.push_back(i);

  std::shuffle(v.begin(), v.end(), g);

  uint64_t lookUpDepths = 0;
  uint64_t treeDepths = 0;
  for (unsigned i = 0; i < Count; i++) {
    // statistics->resetState();
    Avlt::Insert::GoToC0(Allocator, Log, State, v[i], 512);
    // f << i << " " << statistics->getFlushCounter() << std::endl;
    // printf("%u\n", i);
    std::optional<unsigned> foundAt =
        Avlt::getLookupDepth(Allocator, Log.getLog()->Root, v[i]);
    assert(foundAt.has_value());
    lookUpDepths += foundAt.value();
    // treeDepths += getTreeDepth(Allocator, Log.getLog()->Root);
  }
  statistics->dump();
  statistics->resetState();
  printf("assert=%d\n", Avlt::assertTree(Allocator, Log.getLog()->Root));
  printf("depth=%u\n", Avlt::getTreeDepth(Allocator, Log.getLog()->Root));

  printf("lookup=%" PRId64 ", depth=%" PRId64 "\n", lookUpDepths / Count, treeDepths / Count);

  std::shuffle(v.begin(), v.end(), g);

  for (unsigned i = 0; i < Count; i++) {
    Avlt::Remove::GoToC0(Allocator, Log, State, v[i]);
  }
  statistics->dump();
}

static void runInsert(AllocatorType &Allocator, SMStateType &State,
                      LogType &Log) {

  std::random_device rd;
  std::mt19937 g(rd());

  g.seed(Seed);

  std::vector<int> v;
  for (unsigned int i = 0; i < Count; i++)
    v.push_back(i);

  std::shuffle(v.begin(), v.end(), g);
  auto start = std::chrono::high_resolution_clock::now();
  for (unsigned i = 0; i < Count; i++) {
    // printf("insert %d:%d\n", i, v[i]);
    Avlt::Insert::GoToC0(Allocator, Log, State, v[i], 512);
    // assert(Avlt::assertTree(Allocator, Log.getLog()->Root));
    // unsigned TreeSize = Avlt::getTreeSize(Allocator, Log.getLog()->Root);
    // assert(TreeSize == (i + 1));
    // printf("tree size %u\n", TreeSize);
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = stop - start;

  // unsigned TreeSize = Avlt::getTreeSize(Allocator, Log.getLog()->Root);

  // printf("assert=%d nodes=%d\n",
  //       Avlt::assertTree(Allocator, Log.getLog()->Root), TreeSize);
  printf("insert=%fs\n", diff.count());
  // plotNode(Allocator, Log.getLog()->Root);
}

int main(int argc, char **argv) {
#ifdef __linux__
  pin();
#endif

  printCPU();

  statistics = std::make_unique<Statistics>();

#if defined(HAVE_PMDK)
  PMDK::AVLTLogState log;
  PMDK::AVLTAllocator Allocator = {log};
  PMDK::AVLTSMState State;
#else
  DRAM::AVLTLogState log;
  DRAM::AVLTAllocator Allocator = {log};
  DRAM::AVLTSMState State;
#endif

  int opt;
  while ((opt = getopt(argc, argv, "tsrhcd:x:")) != -1) {
    switch (opt) {
    case 's':
      Allocator.resetState();
      State.resetState();
      log.resetState();
      runInsert(Allocator, State, log);
      break;
    case 'r':
      // isRecover = true;
      recoverInsert(Allocator, State, log);
      break;
    case 'd':
      Count = atoi(optarg);
      break;
    case 'x':
      Seed = atoi(optarg);
      break;
    case 't':
      Allocator.resetState();
      State.resetState();
      log.resetState();
      runInsertWithStatistics(Allocator, State, log);
      break;
    case 'c':
      Allocator.resetState();
      State.resetState();
      log.resetState();
      runInsertRemove(Allocator, State, log);
      break;
    case 'h':
    default: /* '?' */
      printf("avl: \n");
      printf(" -d count \n");
      printf(" -s insert \n");
      printf(" -r recover from insert\n");
      printf(" -x seed \n");
      printf(" -t insert with statistics\n");
      printf(" -c insert remove \n");
      exit(EXIT_FAILURE);
    }
  }
  exit(EXIT_SUCCESS);

  static_assert(std::is_standard_layout_v<AVLTLogStructure>);

  return 0;
}
