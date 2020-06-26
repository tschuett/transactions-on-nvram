#include "pmdk-rbt-sm-state.h"

#include <sys/stat.h>

#ifdef HAVE_PMDK

#include "flush.h"
#include "recover.h"

#include <libpmem.h>

#include <cassert>
#include <string>

namespace PMDK {
SMState::SMState() {

  std::string path = "/mnt/pmem0/user1/state.bin";
  size_t size = sizeof(std::atomic<uint64_t>);

  size_t mapped_len = 0;

  data = static_cast<std::atomic<uint64_t> *>(
      pmem_map_file(path.c_str(), size, PMEM_FILE_CREATE, S_IWUSR | S_IRUSR,
                    &mapped_len, nullptr));
  assert(data != nullptr);
  assert(mapped_len == size);
  static_assert(sizeof(std::atomic<uint64_t>) == 8);
  static_assert(sizeof(std::atomic<uint64_t>) == sizeof(StateStructure));
}

void SMState::change(StateStructure from, StateStructure to) {
  if (isRecover)
    printf("%s -> %s\n", StateKind2String(from.State).c_str(),
           StateKind2String(to.State).c_str());
  data->store(to.getAsUint64T(), std::memory_order::memory_order_relaxed);
  flushObj(data, FlushKind::StateChange);
}

void SMState::changeWoEvenOdd(RBTStateKind From, RBTStateKind To) {
  assert(From != To);
  StateStructure State = StateStructureFromUint64T(
      data->load(std::memory_order::memory_order_relaxed));
  assert(State.State == From);

  State.State = To;
  data->store(State.getAsUint64T(), std::memory_order::memory_order_relaxed);
  flushObj(data, FlushKind::StateChange);
}

void SMState::changeWithEvenOdd(RBTStateKind From, RBTStateKind To) {
  assert(From != To);
  StateStructure State = StateStructureFromUint64T(
      data->load(std::memory_order::memory_order_relaxed));
  assert(State.State == From);

  State.State = To;
  State.EvenOdd = getFlippedEvenOddBit();
  data->store(State.getAsUint64T(), std::memory_order::memory_order_relaxed);
  flushObj(data, FlushKind::StateChange);
}

void SMState::changeWithEvenOddSame(RBTStateKind From, RBTStateKind To) {
  StateStructure State = StateStructureFromUint64T(
      data->load(std::memory_order::memory_order_relaxed));
  assert(State.State == From);

  State.State = To;
  State.EvenOdd = getFlippedEvenOddBit();
  data->store(State.getAsUint64T(), std::memory_order::memory_order_relaxed);
  flushObj(data, FlushKind::StateChange);
}

StateStructure SMState::getState() {
  StateStructure State = StateStructureFromUint64T(
      data->load(std::memory_order::memory_order_relaxed));
  return State;
}

void SMState::resetState() {
  new (data) std::atomic<uint64_t>();
  flushObj(data, FlushKind::StateChange);
  StateStructure s = {0, RBTStateKind::C0};
  data->store(s.getAsUint64T(), std::memory_order::memory_order_relaxed);
  flushObj(data, FlushKind::StateChange);
}

RBTStateKind SMState::getStateKind() const {
  StateStructure State = StateStructureFromUint64T(
      data->load(std::memory_order::memory_order_relaxed));
  return State.State;
}

uint32_t SMState::getEvenOddBit() const {
  StateStructure State = StateStructureFromUint64T(
      data->load(std::memory_order::memory_order_relaxed));
  return State.EvenOdd;
}

uint32_t SMState::getFlippedEvenOddBit() const {
  StateStructure State = StateStructureFromUint64T(
      data->load(std::memory_order::memory_order_relaxed));
  if (State.EvenOdd == 0)
    return 1;
  return 0;
}

} // namespace PMDK

#endif
