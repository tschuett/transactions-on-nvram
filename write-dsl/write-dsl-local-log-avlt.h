#pragma once

#include "avlt-allocator-fwd.h"
#include "avlt-log.h"

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

  NodeIndex readValue() { return log.getLog()->Root2; }
  NodeIndex *getAddress() { return &log.getLog()->Root2; }
};

class SaveParentPInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SaveParentPInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->SaveParent; }
  NodeIndex *getAddress() { return &log.getLog()->SaveParent; }
};

class SavePInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SavePInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Save; }
  NodeIndex *getAddress() { return &log.getLog()->Save; }
};

class SaveChildPInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SaveChildPInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->SaveChild; }
  NodeIndex *getAddress() { return &log.getLog()->SaveChild; }
};

class Dir2InLogAddress : public Address {
  LogType &log;

public:
  using type = Direction;

  Dir2InLogAddress(LogType &log) : log(log) {}

  Direction readValue() { return log.getLog()->Dir2; }
  Direction *getAddress() { return &log.getLog()->Dir2; }
};

class Dir3InLogAddress : public Address {
  LogType &log;

public:
  using type = Direction;

  Dir3InLogAddress(LogType &log) : log(log) {}

  Direction readValue() { return log.getLog()->Dir3; }
  Direction *getAddress() { return &log.getLog()->Dir3; }
};

class QPInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  QPInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Q; }
  NodeIndex *getAddress() { return &log.getLog()->Q; }
};

class TpInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  TpInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->T; }
  NodeIndex *getAddress() { return &log.getLog()->T; }
};

class SpInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SpInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->S; }
  NodeIndex *getAddress() { return &log.getLog()->S; }
};

class SPpInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  SPpInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->SP; }
  NodeIndex *getAddress() { return &log.getLog()->SP; }
};

class NInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  NInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->N; }
  NodeIndex *getAddress() { return &log.getLog()->N; }
};

class NNInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  NNInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->NN; }
  NodeIndex *getAddress() { return &log.getLog()->NN; }
};

class PpInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  PpInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->P; }
  NodeIndex *getAddress() { return &log.getLog()->P; }
};

class NextPInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  NextPInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->NextP; }
  NodeIndex *getAddress() { return &log.getLog()->NextP; }
};

class NextSInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  NextSInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->NextS; }
  NodeIndex *getAddress() { return &log.getLog()->NextS; }
};

class NextTInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  NextTInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->NextT; }
  NodeIndex *getAddress() { return &log.getLog()->NextT; }
};

class DirInLogAddress : public Address {
  LogType &log;

public:
  using type = Direction;

  DirInLogAddress(LogType &log) : log(log) {}

  Direction readValue() { return log.getLog()->Dir; }
  Direction *getAddress() { return &log.getLog()->Dir; }
};

class IncInLogAddress : public Address {
  LogType &log;

public:
  using type = int8_t;

  IncInLogAddress(LogType &log) : log(log) {}

  int8_t readValue() { return log.getLog()->Inc; }
  int8_t *getAddress() { return &log.getLog()->Inc; }
};

class BalanceInLogAddress : public Address {
  LogType &log;

public:
  using type = int8_t;

  BalanceInLogAddress(LogType &log) : log(log) {}

  int8_t readValue() { return log.getLog()->Balance; }
  int8_t *getAddress() { return &log.getLog()->Balance; }
};

class Balance2InLogAddress : public Address {
  LogType &log;

public:
  using type = int8_t;

  Balance2InLogAddress(LogType &log) : log(log) {}

  int8_t readValue() { return log.getLog()->Balance2; }
  int8_t *getAddress() { return &log.getLog()->Balance2; }
};

class Balance3InLogAddress : public Address {
  LogType &log;

public:
  using type = int8_t;

  Balance3InLogAddress(LogType &log) : log(log) {}

  int8_t readValue() { return log.getLog()->Balance3; }
  int8_t *getAddress() { return &log.getLog()->Balance3; }
};

class TopInLogAddress : public Address {
  LogType &log;

public:
  using type = int8_t;

  TopInLogAddress(LogType &log) : log(log) {}

  int8_t readValue() { return log.getLog()->Top; }
  int8_t *getAddress() { return &log.getLog()->Top; }
};

class OldTopInLogAddress : public Address {
  LogType &log;

public:
  using type = int8_t;

  OldTopInLogAddress(LogType &log) : log(log) {}

  int8_t readValue() { return log.getLog()->OldTop; }
  int8_t *getAddress() { return &log.getLog()->OldTop; }
};

class DoneInLogAddress : public Address {
  LogType &log;

public:
  using type = bool;

  DoneInLogAddress(LogType &log) : log(log) {}

  bool readValue() { return log.getLog()->Done; }
  bool *getAddress() { return &log.getLog()->Done; }
};

class ItInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  ItInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->It; }
  NodeIndex *getAddress() { return &log.getLog()->It; }
};

class OldItInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  OldItInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->OldIt; }
  NodeIndex *getAddress() { return &log.getLog()->OldIt; }
};

class NewItInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  NewItInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->NewIt; }
  NodeIndex *getAddress() { return &log.getLog()->NewIt; }
};

class NewItUpInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  NewItUpInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->NewItUp; }
  NodeIndex *getAddress() { return &log.getLog()->NewItUp; }
};

class HeirInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  HeirInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Heir; }
  NodeIndex *getAddress() { return &log.getLog()->Heir; }
};

class OldHeirInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  OldHeirInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->OldHeir; }
  NodeIndex *getAddress() { return &log.getLog()->OldHeir; }
};

class OldNewItUpInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  OldNewItUpInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->OldNewItUp; }
  NodeIndex *getAddress() { return &log.getLog()->OldNewItUp; }
};

class NewItDownInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  NewItDownInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->NewItDown; }
  NodeIndex *getAddress() { return &log.getLog()->NewItDown; }
};

class OldNewItInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  OldNewItInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->OldNewIt; }
  NodeIndex *getAddress() { return &log.getLog()->OldNewIt; }
};

class FooInLogAddress : public Address {
  LogType &log;

public:
  using type = NodeIndex;

  FooInLogAddress(LogType &log) : log(log) {}

  NodeIndex readValue() { return log.getLog()->Foo; }
  NodeIndex *getAddress() { return &log.getLog()->Foo; }
};

class FooDirInLogAddress : public Address {
  LogType &log;

public:
  using type = Direction;

  FooDirInLogAddress(LogType &log) : log(log) {}

  Direction readValue() { return log.getLog()->FooDir; }
  Direction *getAddress() { return &log.getLog()->FooDir; }
};

class SaveParentDirInLogAddress : public Address {
  LogType &log;

public:
  using type = Direction;

  SaveParentDirInLogAddress(LogType &log) : log(log) {}

  Direction readValue() { return log.getLog()->SaveParentDir; }
  Direction *getAddress() { return &log.getLog()->SaveParentDir; }
};

class LogAddress {
public:
  using Key = KeyInLogAddress;
  using Value = ValueInLogAddress;
  using Root = RootInLogAddress;
  using Root2P = Root2PInLogAddress;
  using QP = QPInLogAddress;
  using Tp = TpInLogAddress;
  using Pp = PpInLogAddress;
  using Sp = SpInLogAddress;
  using SPp = SPpInLogAddress;
  using SaveParentP = SaveParentPInLogAddress;
  using SaveP = SavePInLogAddress;
  using SaveChildP = SaveChildPInLogAddress;
  using Dir = DirInLogAddress;
  using Dir2 = Dir2InLogAddress;
  using Dir3 = Dir3InLogAddress;
  using NextP = NextPInLogAddress;
  using NextS = NextSInLogAddress;
  using NextT = NextTInLogAddress;
  using Inc = IncInLogAddress;
  using Balance = BalanceInLogAddress;
  using Balance2 = Balance2InLogAddress;
  using Balance3 = Balance3InLogAddress;
  using N = NInLogAddress;
  using NN = NNInLogAddress;
  using Top = TopInLogAddress;
  using OldTop = OldTopInLogAddress;
  using Done = DoneInLogAddress;
  using It = ItInLogAddress;
  using OldIt = OldItInLogAddress;
  using NewIt = NewItInLogAddress;
  using NewItUp = NewItUpInLogAddress;
  using Heir = HeirInLogAddress;
  using OldHeir = OldHeirInLogAddress;
  using OldNewItUp = OldNewItUpInLogAddress;
  using NewItDown = NewItDownInLogAddress;
  using OldNewIt = OldNewItInLogAddress;
  using Foo = FooInLogAddress;
  using FooDir = FooDirInLogAddress;
  using SaveParentDir = SaveParentDirInLogAddress;
};
