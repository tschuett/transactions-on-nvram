#pragma once

#include "rbt-allocator-fwd.h"
#include "rbt-log.h"

class Address {};

class KeyInLogAddress : public Address {
  LogType &log;

public:
  using type = uint64_t;

  KeyInLogAddress(LogType &log) : log(log) {}

  uint64_t readValue() { return log.getLog()->Key; };
  uint64_t *getAddress() { return &log.getLog()->Key; };
};

class ValueInLogAddress : public Address {
  LogType &log;

public:
  using type = uint64_t;

  ValueInLogAddress(LogType &log) : log(log) {}

  uint64_t readValue() { return log.getLog()->Value; };
  uint64_t *getAddress() { return &log.getLog()->Value; };
};

class RootInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  RootInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Root; }
  NodeIndex *getAddress() { return &log.getLog()->Root; }
};

class Root2PInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  Root2PInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Log.Root2P; }
  NodeIndex *getAddress() { return &log.getLog()->Log.Root2P; }
};

class SavePInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SavePInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Log.SaveP; }
  NodeIndex *getAddress() { return &log.getLog()->Log.SaveP; }
};

class SaveChildPInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SaveChildPInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Log.SaveChildP; }
  NodeIndex *getAddress() { return &log.getLog()->Log.SaveChildP; }
};

class Dir2InLogAddress : public Address {
  LogType &log;

public:
  using type = Direction;

  Dir2InLogAddress(LogType &log) : log(log) {}

  Direction readValue() { return log.getLog()->Log.Dir2; }
  Direction *getAddress() { return &log.getLog()->Log.Dir2; }
};

class Dir3InLogAddress : public Address {
  LogType &log;

public:
  using type = Direction;

  Dir3InLogAddress(LogType &log) : log(log) {}

  Direction readValue() { return log.getLog()->Log.Dir3; }
  Direction *getAddress() { return &log.getLog()->Log.Dir3; }
};

class QPInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  QPInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  NodeIndex readValue() { return log.getLog()->Iterator[EvenOdd].QP; }
  NodeIndex *getAddress() { return &log.getLog()->Iterator[EvenOdd].QP; }
};

class Q2PInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  Q2PInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  NodeIndex readValue() { return log.getLog()->Iterator[EvenOdd].Q2P; }
  NodeIndex *getAddress() { return &log.getLog()->Iterator[EvenOdd].Q2P; }
};

class ParentPInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  ParentPInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  NodeIndex readValue() { return log.getLog()->Iterator[EvenOdd].ParentP; }
  NodeIndex *getAddress() { return &log.getLog()->Iterator[EvenOdd].ParentP; }
};

class Parent2PInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  Parent2PInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Log.Parent2P; }
  NodeIndex *getAddress() { return &log.getLog()->Log.Parent2P; }
};

class GrandPInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  GrandPInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  NodeIndex readValue() { return log.getLog()->Iterator[EvenOdd].GrandP; }
  NodeIndex *getAddress() { return &log.getLog()->Iterator[EvenOdd].GrandP; }
};

class TpInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  TpInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  NodeIndex readValue() { return log.getLog()->Iterator[EvenOdd].Tp; }
  NodeIndex *getAddress() { return &log.getLog()->Iterator[EvenOdd].Tp; }
};

class FpInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = NodeIndex;

  FpInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  NodeIndex readValue() { return log.getLog()->Iterator[EvenOdd].Fp; }
  NodeIndex *getAddress() { return &log.getLog()->Iterator[EvenOdd].Fp; }
};

class SpInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SpInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Log.Sp; }
  NodeIndex *getAddress() { return &log.getLog()->Log.Sp; }
};

class DirInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = Direction;

  DirInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  Direction readValue() { return log.getLog()->Iterator[EvenOdd].Dir; }
  Direction *getAddress() { return &log.getLog()->Iterator[EvenOdd].Dir; }
};

class LastDirInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = Direction;

  LastDirInLogAddress(LogType &log, uint8_t EvenOdd) : log(log), EvenOdd(EvenOdd) {}

  Direction readValue() { return log.getLog()->Iterator[EvenOdd].LastDir; }
  Direction *getAddress() { return &log.getLog()->Iterator[EvenOdd].LastDir; }
};

class TmpNodeInLogAddress : public Address {
  LogType &log;

public:
  using type = Node;

  TmpNodeInLogAddress(LogType &log) : log(log) {}

  // Node readValue() { return log.getLog()->Log.TmpNode; }
  Node* getAddress() { return &log.getLog()->Log.TmpNode; }
};

class IteratorsInLogAddress : public Address {
  LogType &log;
  uint8_t EvenOdd;

public:
  using type = Iterators;

  IteratorsInLogAddress(LogType &log, uint8_t EvenOdd)
      : log(log), EvenOdd(EvenOdd) {}

  Iterators readValue() { return log.getLog()->Iterator[EvenOdd]; }
  Iterators *getAddress() { return &log.getLog()->Iterator[EvenOdd]; }
};

class LogAddress {
public:
  using Key = KeyInLogAddress;
  using Value = ValueInLogAddress;
  using Root = RootInLogAddress;
  using Root2P = Root2PInLogAddress;
  using Iterators = IteratorsInLogAddress;
  using QP = QPInLogAddress;
  using Q2P = Q2PInLogAddress;
  using ParentP = ParentPInLogAddress;
  using Parent2P = Parent2PInLogAddress;
  using GrandP = GrandPInLogAddress;
  using Tp = TpInLogAddress;
  using Fp = FpInLogAddress;
  using Sp = SpInLogAddress;
  using SaveP = SavePInLogAddress;
  using SaveChildP = SaveChildPInLogAddress;
  using Dir2 = Dir2InLogAddress;
  using TmpNode = TmpNodeInLogAddress;
  using LastDir = LastDirInLogAddress;
  using Dir = DirInLogAddress;
  using Dir3 = Dir3InLogAddress;
};
