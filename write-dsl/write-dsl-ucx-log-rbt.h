#pragma once

#include "rbt-log.h"
#include "window-kind.h"

class Address {};

class KeyInLogAddress : public Address {
  LogType &log;

public:
  using type = uint64_t;

  KeyInLogAddress(LogType &log) : log(log) {}

  void writeValue(uint64_t Key) { log.writeKey(Key); }

  uint64_t readValue() { return log.readKey(); }

  uint8_t getSize() const { return sizeof(uint64_t); }
  uint32_t getOffset() const { return offsetof(RBTLogStructure, Key); }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class QPInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  QPInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  void writeValue(NodeIndex ni) { log.writeQP(EvenOdd, ni); }

  NodeIndex readValue() { return log.readQP(EvenOdd); }

  uint8_t getSize() const { return sizeof(NodeIndex); };
  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Iterator) + EvenOdd * sizeof(Iterators) +
           offsetof(Iterators, QP);
  }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class RootInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  RootInLogAddress(LogType &log) : log(log) {}

  void writeValue(NodeIndex ni) { log.writeRoot(ni); }

  NodeIndex readValue() { return log.readRoot(); }

  uint8_t getSize() const { return sizeof(NodeIndex); };
  uint32_t getOffset() const { return offsetof(RBTLogStructure, Root); }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class DirInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = Direction;

  DirInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  void writeValue(Direction Dir) { log.writeDir(EvenOdd, Dir); }

  Direction readValue() { return log.readDir(EvenOdd); }

  uint8_t getSize() const { return sizeof(Direction); }
  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Iterator) + EvenOdd * sizeof(Iterators) +
           offsetof(Iterators, Dir);
  }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class ParentPInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  ParentPInLogAddress(LogType &log, uint8_t EvenOdd)
      : log(log), EvenOdd(EvenOdd) {}

  void writeValue(NodeIndex ni) { log.writeParentP(EvenOdd, ni); }

  NodeIndex readValue() { return log.readParentP(EvenOdd); }

  uint8_t getSize() const { return sizeof(NodeIndex); }
  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Iterator) + EvenOdd * sizeof(Iterators) +
           offsetof(Iterators, ParentP);
  }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class Dir2InLogAddress : public Address {
  LogType &log;

public:
  using type = Direction;

  Dir2InLogAddress(LogType &log) : log(log) {}

  void writeValue(Direction Dir) { log.writeDir2(Dir); }

  Direction readValue() { return log.readDir2(); }

  uint8_t getSize() const { return sizeof(Direction); }
  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Dir2);
  }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class IteratorsInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = Iterators;

  IteratorsInLogAddress(LogType &log, uint8_t EvenOdd)
      : log(log), EvenOdd(EvenOdd) {}

  Iterators readValue() { return log.readIterators(EvenOdd); }
  void writeValue(Iterators Its) { log.writeIterators(EvenOdd, Its); }

  // Iterators readValue() { return log.getLog()->Iterator[EvenOdd]; }
  // Iterators *getAddress() { return &log.getLog()->Iterator[EvenOdd]; }

  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Iterator) + EvenOdd * sizeof(Iterators);
  }

  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class Dir3InLogAddress : public Address {
  LogType &log;

public:
  using type = Direction;

  Dir3InLogAddress(LogType &log) : log(log) {}

  Direction readValue() { return log.readDir3(); }
  void writeValue(Direction Dir) { log.writeDir3(Dir); }
  WindowKind getWindow() const { return WindowKind::LogWindow; };

  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Dir3);
  }
};

class FpInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  FpInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  NodeIndex readValue() { return log.readFp(EvenOdd); }

  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class SavePInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SavePInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.readSaveP(); }
  void writeValue(NodeIndex ni) { log.writeSaveP(ni); }

  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Log) + offsetof(RedoLog, SaveP);
  }

  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class SaveChildPInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SaveChildPInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.readSaveChildP(); }
  void writeValue(NodeIndex ni) { log.writeSaveChildP(ni); }

  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Log) + offsetof(RedoLog, SaveChildP);
  }

  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class Parent2PInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  Parent2PInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.readParent2P(); }
  void writeValue(NodeIndex ni) { log.writeParent2P(ni); }

  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Root2P);
  }

  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class GrandPInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  GrandPInLogAddress(LogType &log, uint8_t EvenOdd)
      : log(log), EvenOdd(EvenOdd) {}

  NodeIndex readValue() { return log.readGrandP(EvenOdd); }
  void writeValue(NodeIndex ni) { log.writeGrandP(EvenOdd, ni); }
};

class LastDirInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = Direction;

  LastDirInLogAddress(LogType &log, uint8_t EvenOdd)
      : log(log), EvenOdd(EvenOdd) {}

  Direction readValue() { return log.readLastDir(EvenOdd); }
  void writeValue(Direction Dir) { log.writeLastDir(EvenOdd, Dir); }
};

class SpInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SpInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.readSp(); }
  void writeValue(NodeIndex ni) { log.writeSp(ni); }

  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Sp);
  }

  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class Root2PInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  Root2PInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.readRoot2P(); }
  void writeValue(NodeIndex ni) { log.writeRoot2P(ni); }

  WindowKind getWindow() const { return WindowKind::LogWindow; };

  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Root2P);
  }
};

class ValueInLogAddress : public Address {
  LogType &log;

public:
  using type = uint64_t;

  ValueInLogAddress(LogType &log) : log(log) {}

  void writeValue(uint64_t Value) { log.writeValue(Value); }
  uint64_t readValue() { return log.readValue(); }

  uint8_t getSize() const { return sizeof(uint64_t); }
  uint32_t getOffset() const { return offsetof(RBTLogStructure, Value); }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class Q2PInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  Q2PInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  void writeValue(NodeIndex ni) { log.writeQ2P(EvenOdd, ni); }

  NodeIndex readValue() { return log.readQ2P(EvenOdd); }

  uint8_t getSize() const { return sizeof(NodeIndex); }
  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Iterator) + EvenOdd * sizeof(Iterators) +
           offsetof(Iterators, Q2P);
  }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class TpInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  TpInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  void writeValue(NodeIndex ni) { log.writeTp(EvenOdd, ni); }
  NodeIndex readValue() { return log.readTp(EvenOdd); }

  uint8_t getSize() const { return sizeof(NodeIndex); }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class TmpNodeInLogAddress : public Address {
  LogType &log;

public:
  using type = Node;

  TmpNodeInLogAddress(LogType &log) : log(log) {}

  void writeValue(Node N) { log.writeTmpNode(N); }

  uint8_t getSize() const { return sizeof(Node); };
  uint32_t getOffset() const {
    return offsetof(RBTLogStructure, Log) + offsetof(RedoLog, TmpNode);
  }

  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

class LogAddress {
public:
  using Fp = FpInLogAddress;
  using Key = KeyInLogAddress;
  using Value = ValueInLogAddress;
  using Dir = DirInLogAddress;
  using Dir2 = Dir2InLogAddress;
  using Dir3 = Dir3InLogAddress;
  using LastDir = LastDirInLogAddress;
  using Tp = TpInLogAddress;
  using GrandP = GrandPInLogAddress;
  using ParentP = ParentPInLogAddress;
  using Parent2P = Parent2PInLogAddress;
  using QP = QPInLogAddress;
  using Q2P = Q2PInLogAddress;
  using Root = RootInLogAddress;
  using Root2P = Root2PInLogAddress;
  using TmpNode = TmpNodeInLogAddress;
  // using type = Log;
  using Iterators = IteratorsInLogAddress;
  using SaveP = SavePInLogAddress;
  using SaveChildP = SaveChildPInLogAddress;
  using Sp = SpInLogAddress;
};
