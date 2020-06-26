#pragma once

#include "avlt-log.h"
#include "window-kind.h"

class Address {};

#define ADDRESSTYPE(NAME, TYPE)                                           \
class NAME ## InLogAddress : public Address {                             \
  LogType &log;                                                           \
                                                                          \
public:                                                                   \
  using type = TYPE;                                                      \
                                                                          \
  NAME ## InLogAddress(LogType &log) : log(log) {}                        \
                                                                          \
  void writeValue(TYPE ni) { log.write##NAME(ni); }                       \
                                                                          \
  TYPE readValue() { return log.read##NAME(); }                           \
                                                                          \
  uint8_t getSize() const { return sizeof(TYPE); }                        \
  uint32_t getOffset() const { return offsetof(AVLTLogStructure, NAME); } \
  WindowKind getWindow() const { return WindowKind::LogWindow; };         \
};

ADDRESSTYPE(Key,            uint64_t)
ADDRESSTYPE(Value,          uint64_t)
ADDRESSTYPE(Root,           NodeIndex)
ADDRESSTYPE(Dir,            Direction)
ADDRESSTYPE(Dir2,           Direction)
ADDRESSTYPE(Dir3,           Direction)
ADDRESSTYPE(It,             NodeIndex)
ADDRESSTYPE(NewIt,          NodeIndex)
ADDRESSTYPE(OldIt,          NodeIndex)
ADDRESSTYPE(Balance,        int8_t)
ADDRESSTYPE(Balance2,       int8_t)
ADDRESSTYPE(Balance3,       int8_t)
ADDRESSTYPE(Top,            int8_t)
ADDRESSTYPE(OldTop,         int8_t)
ADDRESSTYPE(Done,           bool)
ADDRESSTYPE(Heir,           NodeIndex)
ADDRESSTYPE(OldHeir,        NodeIndex)
ADDRESSTYPE(NextP,          NodeIndex)
ADDRESSTYPE(NextS,          NodeIndex)
ADDRESSTYPE(NextT,          NodeIndex)
ADDRESSTYPE(S,              NodeIndex)
ADDRESSTYPE(T,              NodeIndex)
ADDRESSTYPE(P,              NodeIndex)
ADDRESSTYPE(Q,              NodeIndex)
ADDRESSTYPE(Inc,            int8_t)
ADDRESSTYPE(NewItDown,      NodeIndex)
ADDRESSTYPE(NewItUp,        NodeIndex)
ADDRESSTYPE(OldNewItUp,     NodeIndex)
ADDRESSTYPE(OldNewIt,       NodeIndex)
ADDRESSTYPE(FooDir,         Direction)
ADDRESSTYPE(Foo,            NodeIndex)
ADDRESSTYPE(N,              NodeIndex)
ADDRESSTYPE(NN,             NodeIndex)
ADDRESSTYPE(SaveChild,      NodeIndex)
ADDRESSTYPE(Save,           NodeIndex)
ADDRESSTYPE(SaveParent,     NodeIndex)
ADDRESSTYPE(SaveParentDir,  Direction)
ADDRESSTYPE(SP,             NodeIndex)
ADDRESSTYPE(Root2,          NodeIndex)


class LogAddress {
public:
  using Key = KeyInLogAddress;
  using Value = ValueInLogAddress;
  using Dir = DirInLogAddress;
  using Dir2 = Dir2InLogAddress;
  using Dir3 = Dir3InLogAddress;
  using Root = RootInLogAddress;
  using Root2P = Root2InLogAddress;
  using It = ItInLogAddress;
  using NewIt = NewItInLogAddress;
  using OldIt = OldItInLogAddress;
  using Top = TopInLogAddress;
  using OldTop = OldTopInLogAddress;
  using Done = DoneInLogAddress;
  using Heir = HeirInLogAddress;
  using OldHeir = OldHeirInLogAddress;
  using NewItUp = NewItUpInLogAddress;
  using OldNewItUp = OldNewItUpInLogAddress;
  using NewItDown = NewItDownInLogAddress;
  using OldNewIt = OldNewItInLogAddress;
  using Balance = BalanceInLogAddress;
  using Balance2 = Balance2InLogAddress;
  using Balance3 = Balance3InLogAddress;
  using N = NInLogAddress;
  using NN = NNInLogAddress;
  using Foo = FooInLogAddress;
  using SaveP = SaveInLogAddress;
  using SaveChildP = SaveChildInLogAddress;
  using SaveParentP = SaveParentInLogAddress;
  using FooDir = FooDirInLogAddress;
  using SaveParentDir = SaveParentDirInLogAddress;
  using Pp = PInLogAddress;
  using Sp = SInLogAddress;
  using Tp = TInLogAddress;
  using QP = QInLogAddress;
  using NextP = NextPInLogAddress;
  using NextS = NextSInLogAddress;
  using NextT = NextTInLogAddress;
  using Inc = IncInLogAddress;
  using SPp = SPInLogAddress;
};
