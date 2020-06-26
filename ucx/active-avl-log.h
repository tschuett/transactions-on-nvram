#pragma once

#include <ucp/api/ucp.h>

#include "avlt-log.h"
#include "tree.h"

namespace ucx::avlt {

class ActiveLog {
  ucp_rkey_h LogKey = nullptr;
  ucp_ep_h ep;
  size_t BaseAddress;

  AVLTLogStructure TheLog;

public:
  ActiveLog(ucp_ep_h ep) : ep(ep) {}

  ~ActiveLog() { ucp_rkey_destroy(LogKey); }

  void init(ucp_rkey_h LogKey_, uintptr_t BaseAddress_);
  void reset();

  void writeRoot(NodeIndex ni);
  void writeKey(uint64_t Key);
  void writeValue(uint64_t Value);

//  void writeDir2(Direction Dir);
//  void writeDir3(Direction Dir);
//  void writeRoot2P(NodeIndex ni);
//  void writeParent2P(NodeIndex ni);
//  void writeSaveP(NodeIndex ni);
//  void writeSaveChildP(NodeIndex ni);
//  void writeSp(NodeIndex ni);
//  void writeTmpNode(Node N);
//
  void writeFooDir(Direction Dir);
  void writeDir(Direction Dir);
  void writeDir2(Direction Dir);
  void writeDir3(Direction Dir);
  void writeIt(NodeIndex ni);
  void writeOldIt(NodeIndex ni);
  void writeOldNewIt(NodeIndex ni);
  void writeOldNewItUp(NodeIndex ni);
  void writeNewItUp(NodeIndex ni);
  void writeNewIt(NodeIndex ni);
  void writeHeir(NodeIndex ni);
  void writeOldHeir(NodeIndex ni);
  void writeTop(int8_t Value);
  void writeOldTop(int8_t Value);
  void writeDone(bool Value);
  void writeBalance(int8_t Value);
  void writeBalance2(int8_t Value);
  void writeBalance3(int8_t Value);
  void writeFoo(NodeIndex ni);
  void writeNewItDown(NodeIndex ni);
  void writeN(NodeIndex ni);
  void writeNN(NodeIndex ni);
  void writeSave(NodeIndex ni);
  void writeSaveChild(NodeIndex ni);
  void writeSaveParent(NodeIndex ni);
  void writeSaveParentDir(Direction ni);
  void writeP(NodeIndex ni);
  void writeS(NodeIndex ni);
  void writeT(NodeIndex ni);
  void writeQ(NodeIndex ni);
  void writeNextP(NodeIndex ni);
  void writeNextS(NodeIndex ni);
  void writeNextT(NodeIndex ni);
  void writeInc(int8_t Value);
  void writeSP(NodeIndex ni);
  void writeRoot2(NodeIndex ni);
//  void writeLastDir(uint8_t EvenOdd, Direction Dir);
//  void writeParentP(uint8_t EvenOdd, NodeIndex ni);
//  void writeQP(uint8_t EvenOdd, NodeIndex ni);
//  void writeGrandP(uint8_t EvenOdd, NodeIndex ni);
//  void writeIterators(uint8_t EvenOdd, Iterators Its);
//  void writeQ2P(uint8_t EvenOdd, NodeIndex ni);
//  void writeTp(uint8_t EvenOdd, NodeIndex ni);
//
  NodeIndex readRoot();
  uint64_t readKey();
  uint64_t readValue();
  NodeIndex readIt();
  NodeIndex readNewIt();
  NodeIndex readNewItUp();
  NodeIndex readNewItDown();
  NodeIndex readOldIt();
  NodeIndex readOldNewIt();
  NodeIndex readOldNewItUp();
  int8_t readTop();
  int8_t readOldTop();
  bool readDone();
  NodeIndex readHeir();
  NodeIndex readOldHeir();
  int8_t readBalance();
  int8_t readBalance2();
  int8_t readBalance3();
  NodeIndex readN();
  NodeIndex readNN();
  NodeIndex readFoo();
  NodeIndex readSave();
  NodeIndex readSaveChild();
  NodeIndex readSaveParent();
  Direction readSaveParentDir();
  NodeIndex readP();
  NodeIndex readS();
  NodeIndex readT();
  NodeIndex readQ();
  NodeIndex readNextP();
  NodeIndex readNextS();
  NodeIndex readNextT();
  int8_t readInc();
  NodeIndex readSP();
  NodeIndex readRoot2();
//  uint64_t readValue();
//
//  Direction readDir2();
//  Direction readDir3();
//  NodeIndex readSaveP();
//  NodeIndex readSaveChildP();
//  NodeIndex readParent2P();
//  NodeIndex readRoot2P();
//  NodeIndex readSp();
//
//  Iterators readIterators(uint8_t EvenOdd);
    Direction readFooDir();
    Direction readDir();
    Direction readDir2();
    Direction readDir3();
//  Direction readLastDir(uint8_t EvenOdd);
//  NodeIndex readQP(uint8_t EvenOdd);
//  NodeIndex readParentP(uint8_t EvenOdd);
//  NodeIndex readGrandP(uint8_t EvenOdd);
//  NodeIndex readFp(uint8_t EvenOdd);
//  NodeIndex readQ2P(uint8_t EvenOdd);
//  NodeIndex readTp(uint8_t EvenOdd);
//
//  NodeIndex readLeftInTmpNode();
//  NodeIndex readRightInTmpNode();
};

} // namespace ucx::rbt
