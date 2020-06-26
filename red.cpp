#include "jsw-common.h"
#include "plot.h"
#include "rbt-allocator.h"
#include "rbt-log.h"
#include "rbt/state-machine-insert.h"
#include "rbt/state-machine-remove.h"
#include "recover.h"
#include "state-kind.h"
#include "statistics.h"
#include "tree.h"
#include "util.h"

#include <sched.h>
#include <unistd.h>

#include <algorithm>
#include <cinttypes>
#include <chrono>
#include <fstream>
#include <memory>
#include <optional>
#include <random>
#include <string>

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
    Rbt::Insert::GoToC0(Allocator, Log, State, v[i], 512);
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = stop - start;
  printf("insert=%fs\n", diff.count());

  // shuffle for remove
  std::shuffle(v.begin(), v.end(), g);

  start = std::chrono::high_resolution_clock::now();
  for (unsigned i = 0; i < Count; i++) {
    Rbt::Remove::GoToC0(Allocator, Log, State, v[i]);
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
  Rbt::Insert::RecoverToC0(Allocator, Log, State);
  printf("assert=%d\n", rb_assert(Allocator, Log, Log.getLog()->Root));
}

static void recoverRemove(AllocatorType &Allocator, SMStateType &State,
                          LogType &Log) {
#ifdef __APPLE__
  printf("recover in %s; key=%llu\n",
         StateKind2String(State.getStateKind()).c_str(), Log.getLog()->Key);
#else
  printf("recover in %s; key=%lu\n",
         StateKind2String(State.getStateKind()).c_str(), Log.getLog()->Key);
#endif
  Rbt::Remove::RecoverToC0(Allocator, Log, State);
  printf("assert=%d\n", rb_assert(Allocator, Log, Log.getLog()->Root));
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
    Rbt::Insert::GoToC0(Allocator, Log, State, v[i], 512);
    // f << i << " " << statistics->getFlushCounter() << std::endl;
    //printf("%u\n", i);
    std::optional<unsigned> foundAt =
        getLookupDepth(Allocator, Log.getLog()->Root, v[i]);
    assert(foundAt.has_value());
    lookUpDepths += foundAt.value();
    //treeDepths += getTreeDepth(Allocator, Log.getLog()->Root);
  }
  statistics->dump();
  statistics->resetState();
  printf("assert=%d\n", rb_assert(Allocator, Log, Log.getLog()->Root));
  printf("depth=%u\n", getTreeDepth(Allocator, Log.getLog()->Root));

  printf("lookup=%" PRId64 ", depth=%" PRId64 "\n", lookUpDepths / Count, treeDepths / Count);
  // for (unsigned i = 0; i < Count; i++) {
  //  Remove::GoToC0(Allocator, Log, State, i);
  //}
  // statistics->dump();
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
    // printf("insert %d\n", i);
    Rbt::Insert::GoToC0(Allocator, Log, State, v[i], 512);
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = stop - start;

  unsigned TreeSize = getTreeSize(Allocator, Log.getLog()->Root);

  printf("assert=%d nodes=%d\n", rb_assert(Allocator, Log, Log.getLog()->Root),
         TreeSize);
  printf("insert=%fs\n", diff.count());
  plotNode(Allocator, Log.getLog()->Root);
}

// static void runRemove(AllocatorType &Allocator, SMStateType &State, LogType
// &Log) {
//  unsigned TreeSize = getTreeSize(Allocator, Log.getLog()->Root);
//  printf("assert=%d nodes=%d\n", rb_assert(Allocator, Log,
//  Log.getLog()->Root),
//         TreeSize);
//  auto start = std::chrono::high_resolution_clock::now();
//  for (unsigned i = 0; i < Count; i++) {
//    Rbt::Remove::GoToC0(Allocator, Log, State, i);
//  }
//  auto stop = std::chrono::high_resolution_clock::now();
//  std::chrono::duration<double> diff = stop - start;
//
//  printf("assert=%d remove=%fs\n",
//         rb_assert(Allocator, Log, Log.getLog()->Root), diff.count());
//}

int main(int argc, char **argv) {
#ifdef __linux__
  pin();
#endif

  printCPU();
  printf("%s\n", VERSION);

  statistics = std::make_unique<Statistics>();

#if defined(HAVE_PMDK)

  PMDK::LogState log;
  PMDK::Allocator Allocator = {log};
  PMDK::SMState State;
#else

  DRAM::LogState log;
  DRAM::Allocator Allocator = {log};
  DRAM::SMState State;
#endif

  printf("sizeof(Tree) = %lu; sizeof(Log) = %lu; sizeof(NodeIndex) = %lu\n",
         sizeof(Node), sizeof(RBTLogStructure), sizeof(NodeIndex));

  int opt;
  while ((opt = getopt(argc, argv, "srcethd:x:")) != -1) {
    switch (opt) {
    case 's':
      Allocator.resetState();
      State.resetState();
      log.resetState();
      runInsert(Allocator, State, log);
      break;
    case 'e':
      isRecover = true;
      recoverRemove(Allocator, State, log);
      break;
    case 'r':
      isRecover = true;
      recoverInsert(Allocator, State, log);
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
    case 'd':
      Count = atoi(optarg);
      break;
    case 'x':
      Seed = atoi(optarg);
      break;
    case 'h':
    default: /* '?' */
      printf("red: \n");
      printf(" -d count \n");
      printf(" -s insert \n");
      printf(" -r recover from insert\n");
      printf(" -e recover from remove\n");
      printf(" -t insert with statistics\n");
      printf(" -c remove\n");
      printf(" -x seed \n");
      exit(EXIT_FAILURE);
    }
  }
  exit(EXIT_SUCCESS);

  static_assert(sizeof(Iterators) == 64);
  static_assert(sizeof(RedoLog) == 64);
}
