#pragma once

#include <ucp/api/ucp.h>

#include "rbt-log.h"
#include "tree.h"

namespace ucx::rbt {

class ActiveLog {
  ucp_rkey_h LogKey = nullptr;
  ucp_ep_h ep;
  size_t BaseAddress;

  RBTLogStructure TheLog;

public:
  ActiveLog(ucp_ep_h ep) : ep(ep) {}

  ~ActiveLog() { ucp_rkey_destroy(LogKey); }

  void init(ucp_rkey_h LogKey_, uintptr_t BaseAddress_);
  void reset();

  void writeRoot(NodeIndex ni);
  void writeKey(uint64_t Key);
  void writeValue(uint64_t Value);

  void writeDir2(Direction Dir);
  void writeDir3(Direction Dir);
  void writeRoot2P(NodeIndex ni);
  void writeParent2P(NodeIndex ni);
  void writeSaveP(NodeIndex ni);
  void writeSaveChildP(NodeIndex ni);
  void writeSp(NodeIndex ni);
  void writeTmpNode(Node N);

  void writeDir(uint8_t EvenOdd, Direction Dir);
  void writeLastDir(uint8_t EvenOdd, Direction Dir);
  void writeParentP(uint8_t EvenOdd, NodeIndex ni);
  void writeQP(uint8_t EvenOdd, NodeIndex ni);
  void writeGrandP(uint8_t EvenOdd, NodeIndex ni);
  void writeIterators(uint8_t EvenOdd, Iterators Its);
  void writeQ2P(uint8_t EvenOdd, NodeIndex ni);
  void writeTp(uint8_t EvenOdd, NodeIndex ni);

  NodeIndex readRoot();
  uint64_t readKey();
  uint64_t readValue();

  Direction readDir2();
  Direction readDir3();
  NodeIndex readSaveP();
  NodeIndex readSaveChildP();
  NodeIndex readParent2P();
  NodeIndex readRoot2P();
  NodeIndex readSp();

  Iterators readIterators(uint8_t EvenOdd);
  Direction readDir(uint8_t EvenOdd);
  Direction readLastDir(uint8_t EvenOdd);
  NodeIndex readQP(uint8_t EvenOdd);
  NodeIndex readParentP(uint8_t EvenOdd);
  NodeIndex readGrandP(uint8_t EvenOdd);
  NodeIndex readFp(uint8_t EvenOdd);
  NodeIndex readQ2P(uint8_t EvenOdd);
  NodeIndex readTp(uint8_t EvenOdd);

  NodeIndex readLeftInTmpNode();
  NodeIndex readRightInTmpNode();
};

} // namespace ucx::rbt
