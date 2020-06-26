#include "state-machine-remove.h"

#include "avlt-allocator.h"
#include "avlt-common.h"
#include "avlt-log.h"
#include "flush.h"
#include "state-kind.h"
#include "tree.h"
#include "write-dsl-avlt.h"

#include <algorithm>
#include <functional>
#include <string>

namespace {

/* flush key and value */
void transitionC0ToDX(LogType &log, SMStateType &State, uint64_t Key) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::Key, KeyValue> W1 = {LogAddress::Key(log), KeyValue(Key)};

  flushOp(FlushKind::FlushCommand, W1);

  // flush([&]() { write(&Log->Key, Key); }, FlushKind::Misc);
}

/* init variables */
void transitionR1ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::Top, Int8Value> W1 = {{log}, Int8Value(0)};
  WriteOp<LogAddress::Done, BooleanValue> W2 = {{log}, BooleanValue(false)};
  WriteOp<LogAddress::It, LogAddress::Root> W3 = {{log}, {log}};
  WriteOp<UpInNode<LogAddress::It>, NodePtrValue> W4 = {{Allocator, {log}},
                                                        NodePtrValue(Nullptr)};
  WriteOp<UpDirInNode<LogAddress::It>, DirectionValue> W5 = {
      {Allocator, {log}}, DirectionValue(Direction::Unknown)};

  flushOp(FlushKind::InitializeVariables, W1, W2, W3, W4, W5);

  // flush(
  //    [&]() {
  //      write(&Log->Top, static_cast<int8_t>(0));
  //      write(&Log->Done, false);
  //      write(&Log->It, Log->Root);
  //      write(&Allocator.get(Log->It)->Up, Nullptr);
  //      write(&Allocator.get(Log->It)->UpDir, Direction::Unknown);
  //    },
  //    FlushKind::Misc);
}

/* push direction and node onto stack (logging) */
void transitionR2ToDX(LogType &log, SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::OldTop, LogAddress::Top> W1 = {{log}, {log}};
  WriteOp<LogAddress::OldIt, LogAddress::It> W2 = {{log}, {log}};

  flushOp(FlushKind::PushStack, W1, W2);

  // flush(
  //    [&]() {
  //      write(&Log->OldTop, Log->Top);
  //      write(&Log->OldIt, Log->It);
  //    },
  //    FlushKind::Misc);
}

/* push direction and node onto stack */
void transitionR3ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  Direction Dir = Direction::Unknown;

  // assert(Log->OldIt != Nullptr);

  if (SmallerThanKey<KeyInNode<LogAddress::OldIt>, LogAddress::Key>(
          {Allocator, {log}}, {log})
          .readValue())

    // if (Allocator.get(Log->OldIt)->key < Log->Key)
    Dir = Direction::Right;
  else
    Dir = Direction::Left;

  NodeIndex NextIt = ChildInNode<LogAddress::OldIt, DirectionValue>(
                         Allocator, DirectionValue(Dir), {log})
                         .readValue();
  // NodeIndex NextIt = Allocator.get(Log->OldIt)->getChild(Dir);

  WriteOp<LogAddress::Top, Int8Value> W1 = {
      {log}, Int8Value(LogAddress::OldTop(log).readValue() + 1)};
  WriteOp<LogAddress::It, NodePtrValue> W2 = {{log}, NodePtrValue(NextIt)};
  WriteOp<UpInNode<NodePtrValue>, LogAddress::OldIt> W3 = {
      {Allocator, NodePtrValue(NextIt)}, {log}};
  WriteOp<UpDirInNode<NodePtrValue>, DirectionValue> W4 = {
      {Allocator, NodePtrValue(NextIt)}, DirectionValue(Dir)};

  flushOp(FlushKind::PushStack, W1, W2, W3, W4);

  // flush(
  //    [&]() {
  //      write(&Log->Top, static_cast<int8_t>(Log->OldTop + 1));
  //      write(&Log->It, NextIt);
  //      write(&Allocator.get(NextIt)->Up, Log->OldIt);
  //      write(&Allocator.get(NextIt)->UpDir, Dir);
  //    },
  //    FlushKind::Misc);
  //
  // assert(Log->OldIt != Nullptr);
}

/* which child is not null */
void transitionR4ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  Direction Dir = Direction::Unknown;
  if (LeftInNode<LogAddress::It>(Allocator, {log}).readValue() == Nullptr)
    // if (Allocator.get(Log->It)->left == Nullptr)
    Dir = Direction::Right;
  else
    Dir = Direction::Left;

  WriteOp<LogAddress::Dir2, DirectionValue> W1 = {{log}, DirectionValue(Dir)};

  flushOp(FlushKind::Misc, W1);

  // flush([&]() { write(&Log->Dir2, Dir); }, FlushKind::Misc);
}

/* fix parent variant 1 */
void transitionR5ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {

  // AVLTLogStructure *Log = log.getLog();

  WriteOp<ChildInNode<UpInNode<LogAddress::It>, UpDirInNode<LogAddress::It>>,
          ChildInNode<LogAddress::It, LogAddress::Dir2>>
      W1 = {{Allocator, {Allocator, {log}}, {Allocator, {log}}},
            {Allocator, {log}, {log}}};

  WriteOp<LogAddress::NewItUp, UpInNode<LogAddress::It>> W2 = {
      {log}, {Allocator, {log}}};
  WriteOp<LogAddress::NewIt, LogAddress::It> W3 = {{log}, {log}};

  flushOp(FlushKind::FixParent, W1, W2, W3);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Allocator.get(Log->It)->Up)
  //                 ->getChild(Allocator.get(Log->It)->UpDir),
  //            Allocator.get(Log->It)->getChild(Log->Dir2));
  //
  //      write(&Log->NewItUp, Allocator.get(Log->It)->Up);
  //      write(&Log->NewIt, Log->It);
  //    },
  //    FlushKind::Misc);
}

/* fix parent variant 2 */
void transitionR6ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::Root, ChildInNode<LogAddress::It, LogAddress::Dir2>> W1 =
      {{log}, {Allocator, {log}, {log}}};
  WriteOp<LogAddress::NewIt, LogAddress::Root> W2 = {{log}, {log}};

  flushOp(FlushKind::FixParent, W1, W2);

  // flush(
  //    [&]() {
  //      write(&Log->Root, Allocator.get(Log->It)->getChild(Log->Dir2));
  //      write(&Log->NewIt, Log->Root);
  //    },
  //    FlushKind::Misc);
}

/* find the inorder successor */
void transitionR7ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::Heir, RightInNode<LogAddress::It>> W1 = {
      {log}, {Allocator, {log}}};

  WriteOp<LogAddress::OldTop, LogAddress::Top> W2 = {{log}, {log}};

  WriteOp<UpInNode<RightInNode<LogAddress::It>>, LogAddress::It> W3 = {
      {Allocator, {Allocator, {log}}}, {log}};

  WriteOp<UpDirInNode<RightInNode<LogAddress::It>>, DirectionValue> W4 = {
      {Allocator, {Allocator, {log}}}, DirectionValue(Direction::Left)};

  flushOp(FlushKind::Misc, W1, W2, W3, W4);

  // flush(
  //    [&]() {
  //      write(&Log->Heir, Allocator.get(Log->It)->right);
  //      write(&Log->OldTop, Log->Top);
  //
  //      write(&Allocator.get(Allocator.get(Log->It)->right)->Up, Log->It);
  //      write(&Allocator.get(Allocator.get(Log->It)->right)->UpDir,
  //            Direction::Left);
  //    },
  //    FlushKind::Misc);
}

/* save the path */
void transitionR8ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::Top, Int8Value> W1 = {
      {log},
      Int8Value(static_cast<int8_t>(LogAddress::OldTop(log).readValue() + 1))};
  WriteOp<UpInNode<LogAddress::Heir>, LogAddress::It> W2 = {{Allocator, {log}},
                                                            {log}};
  WriteOp<UpDirInNode<LogAddress::Heir>, DirectionValue> W3 = {
      {Allocator, {log}}, DirectionValue(Direction::Right)};
  WriteOp<LogAddress::OldHeir, LogAddress::It> W4 = {{log}, {log}};

  flushOp(FlushKind::Misc, W1, W2, W3, W4);

  // flush(
  //    [&]() {
  //      write(&Log->Top, static_cast<int8_t>(Log->OldTop + 1));
  //
  //      write(&Allocator.get(Log->Heir)->Up, Log->It);
  //      write(&Allocator.get(Log->Heir)->UpDir, Direction::Right);
  //      write(&Log->OldHeir, Log->It); // irk for empty while loop
  //    },
  //    FlushKind::Misc);
  //
  // assert(Log->It not_eq Nullptr);
}

/* while loop (logging) */
void transitionR9ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::OldTop, LogAddress::Top> W1 = {{log}, {log}};
  WriteOp<LogAddress::OldHeir, LogAddress::Heir> W2 = {{log}, {log}};

  flushOp(FlushKind::Loop, W1, W2);

  // flush(
  //    [&]() {
  //      write(&Log->OldTop, Log->Top);
  //      write(&Log->OldHeir, Log->Heir);
  //    },
  //    FlushKind::Misc);
}

/* while loop (body) */
void transitionR10ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // assert(Allocator.get(Log->OldHeir)->left != Nullptr);

  WriteOp<LogAddress::Heir, LeftInNode<LogAddress::OldHeir>> W1 = {
      {log}, {Allocator, {log}}};
  WriteOp<LogAddress::Top, Int8Value> W2 = {
      {log},
      Int8Value(static_cast<int8_t>(LogAddress::OldTop(log).readValue() + 1))};
  WriteOp<UpInNode<LogAddress::Heir>, LogAddress::OldHeir> W3 = {
      {Allocator, {log}}, {log}};
  WriteOp<UpDirInNode<LogAddress::Heir>, DirectionValue> W4 = {
      {Allocator, {log}}, DirectionValue(Direction::Left)};

  flushOp(FlushKind::Loop, W1, W2, W3, W4);

  // flush(
  //    [&]() {
  //      write(&Log->Heir, Allocator.get(Log->OldHeir)->left);
  //      write(&Log->Top, static_cast<int8_t>(Log->OldTop + 1));
  //
  //      write(&Allocator.get(Log->Heir)->Up, Log->OldHeir);
  //      write(&Allocator.get(Log->Heir)->UpDir, Direction::Left);
  //    },
  //    FlushKind::Misc);
  //
  // assert(Log->OldHeir != Nullptr);
}

/* swap data */
void transitionR11ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<KeyInNode<LogAddress::It>, KeyInNode<LogAddress::Heir>> W1 = {
      {Allocator, {log}}, {Allocator, {log}}};
  WriteOp<LogAddress::NewIt, LogAddress::Heir> W2 = {{log}, {log}};
  WriteOp<LogAddress::NewItUp, LogAddress::OldHeir> W3 = {{log}, {log}};

  flushOp(FlushKind::Misc, W1, W2, W3);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Log->It)->key, Allocator.get(Log->Heir)->key);
  //
  //      write(&Log->NewIt, Log->Heir);
  //      write(&Log->NewItUp, Log->OldHeir);
  //    },
  //    FlushKind::Misc);
}

/* unlink successor and fix parent (logging) */
void transitionR12ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  Direction Dir = Direction::Unknown;
  //  if (Log->NewItUp == Log->It)
  if (LogAddress::NewItUp(log).readValue() == LogAddress::It(log).readValue())
    Dir = Direction::Right;
  else
    Dir = Direction::Left;

  WriteOp<LogAddress::Dir2, DirectionValue> W1 = {{log}, DirectionValue(Dir)};
  WriteOp<LogAddress::OldIt, LogAddress::NewIt> W2 = {{log}, {log}};

  flushOp(FlushKind::RemoveNode, W1, W2);

  // flush(
  //    [&]() {
  //      write(&Log->Dir2, Dir);
  //      write(&Log->OldIt, Log->NewIt);
  //    },
  //    FlushKind::Misc);
}

/* unlink successor and fix parent */
void transitionR13ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<ChildInNode<LogAddress::NewItUp, LogAddress::Dir2>,
          RightInNode<LogAddress::Heir>>
      W1 = {{Allocator, {log}, {log}}, {Allocator, {log}}};
  WriteOp<UpInNode<RightInNode<LogAddress::Heir>>, LogAddress::OldIt> W2 = {
      {Allocator, {Allocator, {log}}}, {log}};
  WriteOp<UpDirInNode<RightInNode<LogAddress::Heir>>,
          NegateDirection<LogAddress::Dir2>>
      W3 = {{Allocator, {Allocator, {log}}}, {log}};

  if (RightInNode<LogAddress::Heir>(Allocator, {log}).readValue() not_eq
      Nullptr)
    flushOp(FlushKind::RemoveNode, W1, W2, W3);
  else
    flushOp(FlushKind::RemoveNode, W1);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Log->NewItUp)->getChild(Log->Dir2),
  //            Allocator.get(Log->Heir)->right);
  //
  //      if (Allocator.get(Log->Heir)->right != Nullptr) {
  //        write(&Allocator.get(Allocator.get(Log->Heir)->right)->Up,
  //              Log->OldIt);
  //        write(&Allocator.get(Allocator.get(Log->Heir)->right)->UpDir,
  //              negate(Log->Dir2));
  //      }
  //    },
  //    FlushKind::Misc);
}

/* update balance factors (logging) */
void transitionR14ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // assert(Log->OldTop == Log->Top);

  WriteOp<LogAddress::Balance, BalanceInNode<LogAddress::OldNewItUp>> W1 = {
      {log}, {Allocator, {log}}};
  WriteOp<LogAddress::Top, Int8Value> W2 = {
      {log},
      Int8Value(static_cast<int8_t>(LogAddress::OldTop(log).readValue() - 1))};
  WriteOp<LogAddress::NewItUp, UpInNode<LogAddress::OldNewItUp>> W3 = {
      {log}, {Allocator, {log}}};
  WriteOp<LogAddress::NewIt, LogAddress::OldNewItUp> W4 = {{log}, {log}};
  WriteOp<LogAddress::NewItDown, LogAddress::OldNewIt> W5 = {{log}, {log}};

  flushOp(FlushKind::BalanceFactors, W1, W2, W3, W4, W5);

  // flush(
  //    [&]() {
  //      write(&Log->Balance, Allocator.get(Log->OldNewItUp)->Balance);
  //      write(&Log->Top, static_cast<int8_t>(Log->OldTop - 1));
  //      write(&Log->NewItUp, Allocator.get(Log->OldNewItUp)->Up);
  //      write(&Log->NewIt, Log->OldNewItUp);
  //      write(&Log->NewItDown, Log->OldNewIt);
  //    },
  //    FlushKind::Misc);
}

/* fix parent variant 1  */
void transitionR15ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<
      ChildInNode<UpInNode<LogAddress::SaveP>, UpDirInNode<LogAddress::NewIt>>,
      LogAddress::SaveP>
      W1 = {{Allocator, {Allocator, {log}}, {Allocator, {log}}}, {log}};

  flushOp(FlushKind::FixParent, W1);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Allocator.get(Log->Save)->Up)
  //                 ->getChild(Allocator.get(Log->NewIt)->UpDir),
  //            Log->Save);
  //    },
  //    FlushKind::Misc);
}

/* fix parent variant 2  */
void transitionR16ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::Root, LogAddress::SaveP> W1 = {{log}, {log}};

  flushOp(FlushKind::FixParent, W1);

  // flush([&]() { write(&Log->Root, Log->Save); }, FlushKind::Misc);
}

/* update balance factors */
void transitionR17ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  int8_t Diff = 0;
  if (UpDirInNode<LogAddress::NewItDown>(Allocator, {log}).readValue() !=
      Direction::Left)
    // if (Allocator.get(Log->NewItDown)->UpDir != Direction::Left)
    Diff = -1;
  else
    Diff = +1;

  // assert(Log->OldIt != Nullptr);

  WriteOp<BalanceInNode<LogAddress::NewIt>, Int8Value> W1 = {
      {Allocator, {log}},
      Int8Value(
          static_cast<int8_t>(LogAddress::Balance(log).readValue() + Diff))};

  flushOp(FlushKind::BalanceFactors, W1);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Log->NewIt)->Balance,
  //            static_cast<int8_t>(Log->Balance + Diff));
  //    },
  //    FlushKind::Misc);
}

/* RB: init variables */
void transitionR18ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // root = up[top]; dir = upd[top];

  NodeIndex NewFoo = Nullptr;
  Direction NewDir = Direction::Unknown;
  if (LogAddress::NewIt(log).readValue() == LogAddress::Root(log).readValue()) {
    // if (Log->NewIt == Log->Root) {
    // if (Log->NewIt == Log->Root) {
    NewFoo = LogAddress::NewIt(log).readValue();
    // NewFoo = Log->NewIt;
    NewDir = Direction::Unknown;
  } else {
    NewFoo = LogAddress::NewItUp(log).readValue();
    // NewFoo = Log->NewItUp;
    // NewDir = Allocator.get(Log->NewIt)->UpDir;
    NewDir = UpDirInNode<LogAddress::NewIt>(Allocator, {log}).readValue();
  }

  int8_t Bal = 0;
  if (UpDirInNode<LogAddress::NewItDown>(Allocator, {log}).readValue() ==
      Direction::Left)
    // if (Allocator.get(Log->NewItDown)->UpDir == Direction::Left)
    Bal = -1;
  else
    Bal = 1;

  WriteOp<LogAddress::N,
          ChildInNode<LogAddress::NewIt,
                      NegateDirection<UpDirInNode<LogAddress::NewItDown>>>>
      W1 = {{log}, {Allocator, {{Allocator, {log}}}, {log}}};

  WriteOp<LogAddress::Balance, Int8Value> W2 = {{log}, Int8Value(Bal)};
  WriteOp<LogAddress::Foo, NodePtrValue> W3 = {{log}, NodePtrValue(NewFoo)};
  WriteOp<LogAddress::FooDir, DirectionValue> W4 = {{log},
                                                    DirectionValue(NewDir)};

  flushOp(FlushKind::InitializeVariables, W1, W2, W3, W4);

  // flush(
  //    [&]() {
  //      write(&Log->N,
  //            Allocator.get(Log->NewIt)
  //                ->getChild(negate(Allocator.get(Log->NewItDown)->UpDir)));
  //      write(&Log->Balance, Bal);
  //      write(&Log->Foo, NewFoo);
  //      write(&Log->FooDir, NewDir);
  //    },
  //    FlushKind::Misc);
} // namespace

/* RB: update balances */
void transitionR19ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // root = up[top]; dir = upd[top];

  WriteOp<BalanceInNode<LogAddress::NewIt>, Int8Value> W1 = {{Allocator, {log}},
                                                             Int8Value(0)};
  WriteOp<BalanceInNode<LogAddress::N>, Int8Value> W2 = {{Allocator, {log}},
                                                         Int8Value(0)};

  flushOp(FlushKind::Balance, W1, W2);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Log->NewIt)->Balance, static_cast<int8_t>(0));
  //      write(&Allocator.get(Log->N)->Balance, static_cast<int8_t>(0));
  //    },
  //    FlushKind::Misc);
}

/* walk back up the search path (logging) */
void transitionR20ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::OldTop, LogAddress::Top> W1 = {{log}, {log}};
  WriteOp<LogAddress::OldIt, LogAddress::NewIt> W2 = {{log}, {log}};
  WriteOp<LogAddress::OldNewIt, LogAddress::NewIt> W3 = {{log}, {log}};
  WriteOp<LogAddress::OldNewItUp, LogAddress::NewItUp> W4 = {{log}, {log}};

  flushOp(FlushKind::PopStack, W1, W2, W3, W4);

  // flush(
  //    [&]() {
  //      write(&Log->OldTop, Log->Top);
  //      write(&Log->OldIt, Log->NewIt);
  //
  //      write(&Log->OldNewIt, Log->NewIt);
  //      write(&Log->OldNewItUp, Log->NewItUp);
  //    },
  //    FlushKind::Misc);
}

/* RB: update balances II */
void transitionR21ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // root = up[top]; dir = upd[top];

  WriteOp<BalanceInNode<LogAddress::NewIt>, Int8Value> W1 = {
      {Allocator, {log}},
      Int8Value(static_cast<int8_t>(-LogAddress::Balance(log).readValue()))};
  WriteOp<BalanceInNode<LogAddress::N>, LogAddress::Balance> W2 = {
      {Allocator, {log}}, {log}};

  flushOp(FlushKind::Balance, W1, W2);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Log->NewIt)->Balance,
  //            static_cast<int8_t>(-Log->Balance));
  //      write(&Allocator.get(Log->N)->Balance, Log->Balance);
  //    },
  //    FlushKind::Misc);
}

/* RB: logging for single rotation I */
void transitionR22ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // root = up[top]; dir = upd[top];

  NodeIndex Root = LogAddress::NewIt(log).readValue(); // Log->NewIt;
  // NodeIndex Save = Allocator.get(Root)->getChild(
  //    negate(Allocator.get(Log->NewItDown)->UpDir));
  NodeIndex Save =
      ChildInNode<NodePtrValue,
                  NegateDirection<UpDirInNode<LogAddress::NewItDown>>>(
          Allocator, {{Allocator, {log}}}, NodePtrValue(Root))
          .readValue();
  NodeIndex SaveChild =
      ChildInNode<NodePtrValue, UpDirInNode<LogAddress::NewItDown>>(
          Allocator, {Allocator, {log}}, NodePtrValue(Save))
          .readValue();
  // Allocator.get(Save)->getChild(Allocator.get(Log->NewItDown)->UpDir);

  Direction Dir3 = Direction::Unknown;
  // if (Log->OldIt != Nullptr)
  if (LogAddress::OldIt(log).readValue() != Nullptr)
    // Dir3 = Allocator.get(Log->OldIt)->UpDir;
    Dir3 = UpDirInNode<LogAddress::OldIt>(Allocator, {log}).readValue();

  WriteOp<LogAddress::Dir3, DirectionValue> W1 = {{log}, DirectionValue(Dir3)};
  WriteOp<LogAddress::SaveParentP, NodePtrValue> W2 = {{log},
                                                       NodePtrValue(Root)};
  WriteOp<LogAddress::SaveP, NodePtrValue> W3 = {{log}, NodePtrValue(Save)};
  WriteOp<LogAddress::SaveChildP, NodePtrValue> W4 = {{log},
                                                      NodePtrValue(SaveChild)};

  flushOp(FlushKind::SingleRotation, W1, W2, W3, W4);

  // flush(
  //    [&]() {
  //      write(&Log->Dir3, Dir3);
  //      write(&Log->SaveParent, Root);
  //      write(&Log->Save, Save);
  //      write(&Log->SaveChild, SaveChild);
  //    },
  //    FlushKind::Misc);
}

/* RB: single rotation I */
void transitionR23ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // NodeIndex Root = Log->NewIt;
  NodeIndex Root = LogAddress::NewIt(log).readValue();
  Direction Dir =
      UpDirInNode<LogAddress::NewItDown>(Allocator, {log}).readValue();
  // Direction Dir = Allocator.get(Log->NewItDown)->UpDir;
  // root = up[top]; dir = upd[top];

  WriteOp<ChildInNode<NodePtrValue, NegateDirection<DirectionValue>>,
          LogAddress::SaveChildP>
      W1 = {{Allocator, {DirectionValue(Dir)}, NodePtrValue(Root)}, {log}};

  WriteOp<ChildInNode<LogAddress::SaveP, DirectionValue>, NodePtrValue> W2 = {
      {Allocator, DirectionValue(Dir), {log}}, NodePtrValue(Root)};

  WriteOp<UpInNode<LogAddress::SaveP>, UpInNode<LogAddress::SaveParentP>> W3 = {
      {Allocator, {log}}, {Allocator, {log}}};

  // write(&Allocator.get(Root)->Up, Log->Save);
  WriteOp<UpInNode<NodePtrValue>, LogAddress::SaveP> W4 = {
      {Allocator, NodePtrValue(Root)}, {log}};

  WriteOp<UpInNode<LogAddress::SaveChildP>, NodePtrValue> W5 = {
      {Allocator, {log}}, NodePtrValue(Root)};

  WriteOp<UpDirInNode<LogAddress::SaveP>, LogAddress::Dir3> W6 = {
      {Allocator, {log}}, {log}};

  WriteOp<UpDirInNode<LogAddress::Root>, NegateDirection<DirectionValue>> W7 = {
      {Allocator, {log}}, {DirectionValue(Dir)}};

  WriteOp<UpDirInNode<LogAddress::SaveChildP>, DirectionValue> W8 = {
      {Allocator, {log}}, DirectionValue(Dir)};

  if (LogAddress::SaveChildP(log).readValue() not_eq Nullptr)
    flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5, W6, W7, W8);
  else
    flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, /* W5,*/ W6, W7 /*, W8*/);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Root)->getChild(negate(Dir)), Log->SaveChild);
  //      write(&Allocator.get(Log->Save)->getChild(Dir), Root);
  //      /* up links */
  //      write(&Allocator.get(Log->Save)->Up,
  //            Allocator.get(Log->SaveParent)->Up);
  //      write(&Allocator.get(Root)->Up, Log->Save);
  //      if (Log->SaveChild != Nullptr)
  //        write(&Allocator.get(Log->SaveChild)->Up, Root);
  //      /* up directions */
  //      write(&Allocator.get(Log->Save)->UpDir, Log->Dir3);
  //      write(&Allocator.get(Log->Root)->UpDir, negate(Dir));
  //      if (Log->SaveChild != Nullptr)
  //        write(&Allocator.get(Log->SaveChild)->UpDir, Dir);
  //    },
  //    FlushKind::Misc);

  /*
    Save->Up = SaveParent
    Root->Up = Log->Save
    SaveChild->Up = Root
   */

  /*
    Save->UpDir =
    Root->UpDir = negate(Dir)
    SaveChild->UpDir = Dir
   */
}

/* RB: logging for first single in double rotation */
void transitionR24ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // root = up[top]->getChild(negate(upd[top])); dir = negate(upd[top]);
  // Direction Dir = negate(Allocator.get(Log->NewItDown)->UpDir);
  Direction Dir =
      NegateDirection<UpDirInNode<LogAddress::NewItDown>>({Allocator, {log}})
          .readValue();
  // NodeIndex Root = Allocator.get(Log->NewIt)->getChild(Dir);
  NodeIndex Root = ChildInNode<LogAddress::NewIt, DirectionValue>(
                       Allocator, DirectionValue(Dir), {log})
                       .readValue();

  NodeIndex Save = ChildInNode<NodePtrValue, NegateDirection<DirectionValue>>(
                       Allocator, {DirectionValue(Dir)}, NodePtrValue(Root))
                       .readValue();
  // NodeIndex Save = Allocator.get(Root)->getChild(negate(Dir));
  // NodeIndex SaveChild = Allocator.get(Save)->getChild(Dir);
  NodeIndex SaveChild = ChildInNode<NodePtrValue, DirectionValue>(
                            Allocator, DirectionValue(Dir), NodePtrValue(Save))
                            .readValue();

  Direction Dir3 = Direction::Unknown;
  if (LogAddress::OldIt(log).readValue() != Nullptr)
    // if (Log->OldIt != Nullptr)
    Dir3 = UpDirInNode<LogAddress::OldIt>(Allocator, {log}).readValue();
  // Dir3 = Allocator.get(Log->OldIt)->UpDir;

  WriteOp<LogAddress::Dir3, DirectionValue> W1 = {{log}, DirectionValue(Dir3)};
  WriteOp<LogAddress::SaveParentP, NodePtrValue> W2 = {{log},
                                                       NodePtrValue(Root)};
  WriteOp<LogAddress::SaveP, NodePtrValue> W3 = {{log}, NodePtrValue(Save)};
  WriteOp<LogAddress::SaveChildP, NodePtrValue> W4 = {{log},
                                                      NodePtrValue(SaveChild)};

  flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4);

  // flush(
  //    [&]() {
  //      write(&Log->Dir3, Dir3);
  //      write(&Log->SaveParent, Root);
  //      write(&Log->Save, Save);
  //      write(&Log->SaveChild, SaveChild);
  //    },
  //    FlushKind::Misc);
}

/* RB: first single in double rotation */
void transitionR25ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // Direction Dir = negate(Allocator.get(Log->NewItDown)->UpDir);
  Direction Dir =
      NegateDirection<UpDirInNode<LogAddress::NewItDown>>({Allocator, {log}})
          .readValue();
  // NodeIndex Root = Allocator.get(Log->NewIt)->getChild(Dir);
  NodeIndex Root = ChildInNode<LogAddress::NewIt, DirectionValue>(
                       Allocator, DirectionValue(Dir), {log})
                       .readValue();

  WriteOp<ChildInNode<NodePtrValue, NegateDirection<DirectionValue>>,
          LogAddress::SaveChildP>
      W1 = {{Allocator, {DirectionValue(Dir)}, NodePtrValue(Root)}, {log}};
  WriteOp<ChildInNode<LogAddress::SaveP, DirectionValue>, NodePtrValue> W2 = {
      {Allocator, {DirectionValue(Dir)}, {log}}, NodePtrValue(Root)};
  WriteOp<ChildInNode<LogAddress::NewIt, DirectionValue>, LogAddress::SaveP>
      W3 = {{Allocator, {DirectionValue(Dir)}, {log}}, {log}};
  WriteOp<UpInNode<LogAddress::SaveP>, LogAddress::SaveParentP> W4 = {
      {Allocator, {log}}, {log}};
  WriteOp<UpInNode<NodePtrValue>, LogAddress::SaveP> W5 = {
      {Allocator, NodePtrValue(Root)}, {log}};
  WriteOp<UpInNode<LogAddress::SaveChildP>, NodePtrValue> W6 = {
      {Allocator, {log}}, NodePtrValue(Root)};
  WriteOp<UpDirInNode<LogAddress::SaveP>, NegateDirection<DirectionValue>> W7 =
      {{Allocator, {log}}, {DirectionValue(Dir)}};
  WriteOp<UpDirInNode<LogAddress::Root>, NegateDirection<DirectionValue>> W8 = {
      {Allocator, {log}}, {DirectionValue(Dir)}};
  WriteOp<UpDirInNode<LogAddress::SaveChildP>, DirectionValue> W9 = {
      {Allocator, {log}}, DirectionValue(Dir)};

  if (LogAddress::SaveChildP(log).readValue() != Nullptr)
    flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5, W6, W7, W8, W9);
  else
    flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5, /*W6,*/ W7,
            W8 /*, W9*/);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Root)->getChild(negate(Dir)), Log->SaveChild);
  //      write(&Allocator.get(Log->Save)->getChild(Dir), Root);
  //      write(&Allocator.get(Log->NewIt)->getChild(Dir), Log->Save);
  //      /* up links */
  //      write(&Allocator.get(Log->Save)->Up, Log->SaveParent);
  //      write(&Allocator.get(Root)->Up, Log->Save);
  //      if (Log->SaveChild != Nullptr)
  //        write(&Allocator.get(Log->SaveChild)->Up, Root);
  //      /* up directions */
  //      write(&Allocator.get(Log->Save)->UpDir, negate(Dir));
  //      write(&Allocator.get(Log->Root)->UpDir, negate(Dir));
  //      if (Log->SaveChild != Nullptr)
  //        write(&Allocator.get(Log->SaveChild)->UpDir, Dir);
  //    },
  //    FlushKind::Misc);

  /*
    Save->Up = SaveParent
    Root->Up = Log->Save
    SaveChild->Up = Root
   */

  /*
    Save->UpDir = negate(Dir)
    Root->UpDir = negate(Dir)
    SaveChild->UpDir = Dir
   */
}

/* RB: logging for second single in double rotation */
void transitionR26ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  NodeIndex Root = LogAddress::NewIt(log).readValue(); // Log->NewIt;
  // Direction Dir = Allocator.get(Log->NewItDown)->UpDir;
  Direction Dir =
      UpDirInNode<LogAddress::NewItDown>(Allocator, {log}).readValue();
  // root = up[top]; dir = upd[top];

  // NodeIndex Save = Allocator.get(Root)->getChild(negate(Dir));
  NodeIndex Save = ChildInNode<NodePtrValue, NegateDirection<DirectionValue>>(
                       Allocator, {DirectionValue(Dir)}, NodePtrValue(Root))
                       .readValue();
  // NodeIndex SaveChild = Allocator.get(Save)->getChild(Dir);
  NodeIndex SaveChild = ChildInNode<NodePtrValue, DirectionValue>(
                            Allocator, DirectionValue(Dir), NodePtrValue(Save))
                            .readValue();

  Direction Dir3 = Direction::Unknown;
  if (LogAddress::OldIt(log).readValue() != Nullptr)
    // if (Log->OldIt != Nullptr)
    Dir3 = UpDirInNode<LogAddress::OldIt>(Allocator, {log}).readValue();
  // Dir3 = Allocator.get(Log->OldIt)->UpDir;

  WriteOp<LogAddress::Dir3, DirectionValue> W1 = {{log}, DirectionValue(Dir3)};
  WriteOp<LogAddress::SaveParentP, LogAddress::Foo> W2 = {{log}, {log}};
  WriteOp<LogAddress::SaveParentDir, LogAddress::FooDir> W3 = {{log}, {log}};
  WriteOp<LogAddress::SaveP, NodePtrValue> W4 = {{log}, NodePtrValue(Save)};
  WriteOp<LogAddress::SaveChildP, NodePtrValue> W5 = {{log},
                                                      NodePtrValue(SaveChild)};

  flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);

  // flush(
  //    [&]() {
  //      write(&Log->Dir3, Dir3);
  //      write(&Log->SaveParent, Log->Foo);
  //      write(&Log->SaveParentDir, Log->FooDir);
  //      write(&Log->Save, Save);
  //      write(&Log->SaveChild, SaveChild);
  //    },
  //    FlushKind::Misc);
}

/* RB: second single in double rotation */
void transitionR27ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  NodeIndex Root = LogAddress::NewIt(log).readValue(); // Log->NewIt;
  // Direction Dir = Allocator.get(Log->NewItDown)->UpDir;
  Direction Dir =
      UpDirInNode<LogAddress::NewItDown>(Allocator, {log}).readValue();
  // root = up[top]; dir = upd[top];

  WriteOp<ChildInNode<NodePtrValue, NegateDirection<DirectionValue>>,
          LogAddress::SaveChildP>
      W1 = {{Allocator, {DirectionValue(Dir)}, NodePtrValue(Root)}, {log}};
  WriteOp<ChildInNode<LogAddress::SaveP, DirectionValue>, NodePtrValue> W2 = {
      {Allocator, DirectionValue(Dir), {log}}, NodePtrValue(Root)};
  WriteOp<UpInNode<LogAddress::SaveP>, LogAddress::SaveParentP> W3 = {
      {Allocator, {log}}, {log}};
  WriteOp<UpInNode<NodePtrValue>, LogAddress::SaveP> W4 = {
      {Allocator, NodePtrValue(Root)}, {log}};
  WriteOp<UpInNode<LogAddress::SaveChildP>, NodePtrValue> W5 = {
      {Allocator, {log}}, NodePtrValue(Root)};
  WriteOp<UpDirInNode<LogAddress::SaveP>, LogAddress::SaveParentDir> W6 = {
      {Allocator, {log}}, {log}};
  WriteOp<UpDirInNode<LogAddress::Root>, NegateDirection<DirectionValue>> W7 = {
      {Allocator, {log}}, {DirectionValue(Dir)}};
  WriteOp<UpDirInNode<LogAddress::SaveChildP>, DirectionValue> W8 = {
      {Allocator, {log}}, DirectionValue(Dir)};

  if (LogAddress::SaveChildP(log).readValue() != Nullptr)
    flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5, W6, W7, W8);
  else
    flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, /*W5*/ W6, W7 /*, W8*/);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Root)->getChild(negate(Dir)), Log->SaveChild);
  //      write(&Allocator.get(Log->Save)->getChild(Dir), Root);
  //      /* up links */
  //      write(&Allocator.get(Log->Save)->Up, Log->SaveParent);
  //      write(&Allocator.get(Root)->Up, Log->Save);
  //      if (Log->SaveChild != Nullptr)
  //        write(&Allocator.get(Log->SaveChild)->Up, Root);
  //      /* up directions */
  //      write(&Allocator.get(Log->Save)->UpDir, Log->SaveParentDir);
  //      write(&Allocator.get(Log->Root)->UpDir, negate(Dir));
  //      if (Log->SaveChild != Nullptr)
  //        write(&Allocator.get(Log->SaveChild)->UpDir, Dir);
  //    },
  //    FlushKind::Misc);

  /*
    Save->Up = SaveParent
    Root->Up = Save
    SaveChild->Up = Root
   */

  // FIXME Directions?
  /*
    Save->UpDir = SaveParentDir
    Root->UpDir = negate(Dir)
    SaveChild->UpDir = Dir
   */
}

/* RB: logging for single rotation II */
void transitionR28ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  // root = up[top]; dir = upd[top];

  // NodeIndex Save = Allocator.get(Log->NewIt)
  //                     ->getChild(negate(Allocator.get(Log->NewItDown)->UpDir));

  NodeIndex Save =
      ChildInNode<LogAddress::NewIt,
                  NegateDirection<UpDirInNode<LogAddress::NewItDown>>>(
          Allocator, {{Allocator, {log}}}, {log})
          .readValue();

  // NodeIndex SaveChild =
  //    Allocator.get(Save)->getChild(Allocator.get(Log->NewItDown)->UpDir);

  NodeIndex SaveChild =
      ChildInNode<NodePtrValue, UpDirInNode<LogAddress::NewItDown>>(
          Allocator, {Allocator, {log}}, NodePtrValue(Save))
          .readValue();

  WriteOp<LogAddress::Dir3, UpDirInNode<LogAddress::NewIt>> W1 = {
      {log}, {Allocator, {log}}};
  WriteOp<LogAddress::SaveParentP, LogAddress::NewItUp> W2 = {{log}, {log}};
  WriteOp<LogAddress::SaveP, NodePtrValue> W3 = {{log}, NodePtrValue(Save)};
  WriteOp<LogAddress::SaveChildP, NodePtrValue> W4 = {{log},
                                                      NodePtrValue(SaveChild)};

  flushOp(FlushKind::SingleRotation, W1, W2, W3, W4);

  // flush(
  //    [&]() {
  //      write(&Log->Dir3, Allocator.get(Log->NewIt)->UpDir);
  //      write(&Log->SaveParent, Log->NewItUp);
  //      write(&Log->Save, Save);
  //      write(&Log->SaveChild, SaveChild);
  //    },
  //    FlushKind::Misc);
}

/* RB: single rotation II */
void transitionR29ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // NodeIndex Root = Log->NewIt;
  NodeIndex Root = LogAddress::NewIt(log).readValue();
  // Direction Dir = Allocator.get(Log->NewItDown)->UpDir;
  Direction Dir =
      UpDirInNode<LogAddress::NewItDown>(Allocator, {log}).readValue();
  // root = up[top]; dir = upd[top];

  WriteOp<ChildInNode<NodePtrValue, NegateDirection<DirectionValue>>,
          LogAddress::SaveChildP>
      W1 = {{Allocator, {DirectionValue(Dir)}, NodePtrValue(Root)}, {log}};

  WriteOp<ChildInNode<LogAddress::SaveP, DirectionValue>, NodePtrValue> W2 = {
      {Allocator, DirectionValue(Dir), {log}}, NodePtrValue(Root)};

  WriteOp<LogAddress::Done, BooleanValue> W3 = {{log}, BooleanValue(true)};

  WriteOp<UpInNode<LogAddress::SaveChildP>, NodePtrValue> W4 = {
      {Allocator, {log}}, NodePtrValue(Root)};

  WriteOp<UpInNode<LogAddress::SaveP>, LogAddress::SaveParentP> W5 = {
      {Allocator, {log}}, {log}};

  WriteOp<UpDirInNode<LogAddress::SaveP>, LogAddress::Dir3> W6 = {
      {Allocator, {log}}, {log}};

  WriteOp<UpDirInNode<LogAddress::Root>, NegateDirection<DirectionValue>> W7 = {
      {Allocator, {log}}, {DirectionValue(Dir)}};

  WriteOp<UpDirInNode<LogAddress::SaveChildP>, DirectionValue> W8 = {
      {Allocator, {log}}, {DirectionValue(Dir)}};

  if (LogAddress::SaveChildP(log).readValue() != Nullptr)
    flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5, W6, W7, W8);
  else
    flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5, W6, W7 /*, W8*/);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Root)->getChild(negate(Dir)), Log->SaveChild);
  //      write(&Allocator.get(Log->Save)->getChild(Dir), Root);
  //      write(&Log->Done, true);
  //      /* update up pointers */
  //      write(&Allocator.get(Root)->Up, Log->Save);
  //      write(&Allocator.get(Log->SaveChild)->Up, Root);
  //      write(&Allocator.get(Log->Save)->Up, Log->SaveParent);
  //      /* up directions */
  //      write(&Allocator.get(Log->Save)->UpDir, Log->Dir3);
  //      write(&Allocator.get(Log->Root)->UpDir, negate(Dir));
  //      if (Log->SaveChild != Nullptr)
  //        write(&Allocator.get(Log->SaveChild)->UpDir, Dir);
  //    },
  //    FlushKind::Misc);

  /*
  Root->Up = Save
  SaveChild->Up = Root
  Save->Up = SaveParent
  */

  /*
    Save->UpDir = Dir3
    Root->UpDir = negate(Dir)
    SaveChild->UpDir = Dir
   */
}

/* AB: init variables */
void transitionR30ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  // root = up[top]; dir = not upd[top]; bal = -Log->Balance

  WriteOp<LogAddress::N,
          ChildInNode<LogAddress::NewIt,
                      NegateDirection<UpDirInNode<LogAddress::NewItDown>>>>
      W1 = {{log}, {Allocator, {{Allocator, {log}}}, {log}}};

  WriteOp<LogAddress::NN,
          ChildInNode<LogAddress::N, UpDirInNode<LogAddress::NewItDown>>>
      W2 = {{log}, {Allocator, {Allocator, {log}}, {log}}};

  flushOp(FlushKind::InitializeVariables, W1, W2);

  // flush(
  //    [&]() {
  //      write(&Log->N,
  //            Allocator.get(Log->NewIt)
  //                ->getChild(negate(Allocator.get(Log->NewItDown)->UpDir)));
  //      write(&Log->NN, Allocator.get(Log->N)->getChild(
  //                          Allocator.get(Log->NewItDown)->UpDir));
  //    },
  //    FlushKind::Misc);
}

/* AB: update balance I */
void transitionR31ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<BalanceInNode<LogAddress::NewIt>, Int8Value> W1 = {{Allocator, {log}},
                                                             Int8Value(0)};
  WriteOp<BalanceInNode<LogAddress::N>, Int8Value> W2 = {{Allocator, {log}},
                                                         Int8Value(0)};

  flushOp(FlushKind::Balance, W1, W2);

  // root = up[top]; dir = not upd[top]; bal = -Log->Balance
  // flush(
  //    [&]() {
  //      write(&Allocator.get(Log->NewIt)->Balance, static_cast<int8_t>(0));
  //      write(&Allocator.get(Log->N)->Balance, static_cast<int8_t>(0));
  //    },
  //    FlushKind::Misc);
}

/* AB: update balance II */
void transitionR32ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // root = up[top]; dir = not upd[top]; bal = -Log->Balance

  WriteOp<BalanceInNode<LogAddress::NewIt>, LogAddress::Balance> W1 = {
      {Allocator, {log}}, {log}};
  WriteOp<BalanceInNode<LogAddress::N>, Int8Value> W2 = {{Allocator, {log}},
                                                         Int8Value(0)};

  flushOp(FlushKind::Balance, W1, W2);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Log->NewIt)->Balance, Log->Balance);
  //      write(&Allocator.get(Log->N)->Balance, static_cast<int8_t>(0));
  //    },
  //    FlushKind::Misc);
}

/* AB: update balance III */
void transitionR33ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // root = up[top]; dir = not upd[top]; bal = -Log->Balance

  WriteOp<BalanceInNode<LogAddress::NewIt>, Int8Value> W1 = {{Allocator, {log}},
                                                             Int8Value(0)};
  WriteOp<BalanceInNode<LogAddress::N>, NegateBalance<LogAddress::Balance>> W2 =
      {{Allocator, {log}}, {{log}}};

  flushOp(FlushKind::Balance, W1, W2);

  // flush(
  //     [&]() {
  //       write(&Allocator.get(Log->NewIt)->Balance, static_cast<int8_t>(0));
  //       write(&Allocator.get(Log->N)->Balance,
  //             static_cast<int8_t>(-Log->Balance));
  //     },
  //     FlushKind::Misc);
}

/* AB: final statement */
void transitionR34ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // root = up[top]; dir = not upd[top]; bal = -Log->Balance

  WriteOp<BalanceInNode<LogAddress::NN>, Int8Value> W1 = {{Allocator, {log}},
                                                          Int8Value(0)};

  flushOp(FlushKind::Misc, W1);

  // flush(
  //     [&]() {
  //       write(&Allocator.get(Log->NN)->Balance, static_cast<int8_t>(0));
  //     },
  //     FlushKind::Misc);
}

void processStateC0(LogType &log, SMStateType &State, uint64_t Key) {
  // AVLTLogStructure *Log = log.getLog();
  transitionC0ToDX(log, State, Key);

  if (IsNullPtr<LogAddress::Root>(log).readValue())
    // if (Log->Root == Nullptr)
    State.changeWoEvenOdd(AVLTStateKind::C0, AVLTStateKind::C0);
  else
    State.changeWoEvenOdd(AVLTStateKind::C0, AVLTStateKind::R1);
}

void processStateR1(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR1ToDX(Allocator, log, State);

  if (IsNullPtr<LogAddress::It>(log).readValue())
    // if (Log->It == Nullptr)
    State.changeWoEvenOdd(AVLTStateKind::R1, AVLTStateKind::C0);
  // else if (Allocator.get(Log->It)->key == Log->Key) {
  else if (EqualKey<KeyInNode<LogAddress::It>, LogAddress::Key>(
               {Allocator, {log}}, {log})
               .readValue()) {
    // break
    if (IsNullPtr<LeftInNode<LogAddress::It>>({Allocator, {log}}).readValue() or
        IsNullPtr<RightInNode<LogAddress::It>>({Allocator, {log}}).readValue())
      // if ((Allocator.get(Log->It)->left == Nullptr) or
      //    (Allocator.get(Log->It)->right == Nullptr))
      State.changeWoEvenOdd(AVLTStateKind::R1, AVLTStateKind::R4);
    else
      State.changeWoEvenOdd(AVLTStateKind::R1, AVLTStateKind::R7);
  } else
    State.changeWoEvenOdd(AVLTStateKind::R1, AVLTStateKind::R2);
}

void processStateR2(LogType &log, SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR2ToDX(log, State);

  State.changeWoEvenOdd(AVLTStateKind::R2, AVLTStateKind::R3);
}

void processStateR3(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR3ToDX(Allocator, log, State);

  if (IsNullPtr<LogAddress::It>(log).readValue())
    // if (Log->It == Nullptr)
    State.changeWoEvenOdd(AVLTStateKind::R3, AVLTStateKind::C0);
  else if (KeyInNode<LogAddress::It>(Allocator, {log}).readValue() ==
           LogAddress::Key(log).readValue()) {
    // else if (Allocator.get(Log->It)->key == Log->Key) {
    // break
    if (IsNullPtr<LeftInNode<LogAddress::It>>({Allocator, {log}}).readValue() or
        IsNullPtr<RightInNode<LogAddress::It>>({Allocator, {log}}).readValue())
      // if ((Allocator.get(Log->It)->left == Nullptr) or
      //    (Allocator.get(Log->It)->right == Nullptr))
      State.changeWoEvenOdd(AVLTStateKind::R3, AVLTStateKind::R4);
    else
      State.changeWoEvenOdd(AVLTStateKind::R3, AVLTStateKind::R7);
  } else
    State.changeWoEvenOdd(AVLTStateKind::R3, AVLTStateKind::R2);
}

void processStateR4(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR4ToDX(Allocator, log, State);

  if (LogAddress::Top(log).readValue() != 0)
    // if (Log->Top != 0)
    State.changeWoEvenOdd(AVLTStateKind::R4, AVLTStateKind::R5);
  else
    State.changeWoEvenOdd(AVLTStateKind::R4, AVLTStateKind::R6);
}

void processStateR5(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR5ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R5, AVLTStateKind::R20);
}

void processStateR6(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR6ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R6, AVLTStateKind::R20);
}

void processStateR7(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR7ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R7, AVLTStateKind::R8);
}

void processStateR8(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR8ToDX(Allocator, log, State);

  if (LeftInNode<LogAddress::Heir>(Allocator, {log}).readValue() != Nullptr)
    // if (Allocator.get(Log->Heir)->left != Nullptr)
    State.changeWoEvenOdd(AVLTStateKind::R8, AVLTStateKind::R9);
  else
    State.changeWoEvenOdd(AVLTStateKind::R8, AVLTStateKind::R11);
}

void processStateR9(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR9ToDX(Allocator, log, State);

  if (LeftInNode<LogAddress::Heir>(Allocator, {log}).readValue() != Nullptr)
    // if (Allocator.get(Log->Heir)->left != Nullptr)
    State.changeWoEvenOdd(AVLTStateKind::R9, AVLTStateKind::R10);
  else
    State.changeWoEvenOdd(AVLTStateKind::R9, AVLTStateKind::R11);
}

void processStateR10(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR10ToDX(Allocator, log, State);

  // assert(Log->Heir != Nullptr);

  // if (Allocator.get(Log->Heir)->left != Nullptr)
  if (LeftInNode<LogAddress::Heir>(Allocator, {log}).readValue() != Nullptr)
    State.changeWoEvenOdd(AVLTStateKind::R10, AVLTStateKind::R9);
  else
    State.changeWoEvenOdd(AVLTStateKind::R10, AVLTStateKind::R11);
}

void processStateR11(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR11ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R11, AVLTStateKind::R12);
}

void processStateR12(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR12ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R12, AVLTStateKind::R13);
}

void processStateR13(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR13ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R13, AVLTStateKind::R20);
}

void processStateR14(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR14ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R14, AVLTStateKind::R17);
}

void processStateR15(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR15ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R15, AVLTStateKind::R20);
}

void processStateR16(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR16ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R16, AVLTStateKind::R20);
}

void processStateR17(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR17ToDX(Allocator, log, State);

  // assert(Log->Up[Log->Top] == Log->NewIt);

  if (std::abs(
          BalanceInNode<LogAddress::NewIt>(Allocator, {log}).readValue()) == 1)
    // if (std::abs(Allocator.get(Log->NewIt)->Balance) == 1)
    State.changeWoEvenOdd(AVLTStateKind::R17, AVLTStateKind::C0);
  else if (std::abs(
               BalanceInNode<LogAddress::NewIt>(Allocator, {log}).readValue()) >
           1)
    // else if (std::abs(Allocator.get(Log->NewIt)->Balance) > 1)
    State.changeWoEvenOdd(AVLTStateKind::R17, AVLTStateKind::R18);
  else
    State.changeWoEvenOdd(AVLTStateKind::R17, AVLTStateKind::R20);
}

void processStateR18(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR18ToDX(Allocator, log, State);

  // BalanceInNode<LogAddress::NewIt>(Allocator, {log}).readValue()

  // if (Allocator.get(Log->N)->Balance == -Log->Balance)
  if (BalanceInNode<LogAddress::N>(Allocator, {log}).readValue() ==
      NegateBalance<LogAddress::Balance>(log).readValue())
    State.changeWoEvenOdd(AVLTStateKind::R18, AVLTStateKind::R19);
  else if (BalanceInNode<LogAddress::N>(Allocator, {log}).readValue() ==
           LogAddress::Balance(log).readValue())
    // else if (Allocator.get(Log->N)->Balance == Log->Balance)
    State.changeWoEvenOdd(AVLTStateKind::R18, AVLTStateKind::R30);
  else /* n->balance == 0 */
    State.changeWoEvenOdd(AVLTStateKind::R18, AVLTStateKind::R21);
}

void processStateR19(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR19ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R19, AVLTStateKind::R22);
}

void processStateR20(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR20ToDX(Allocator, log, State);

  if (((LogAddress::OldTop(log).readValue() - 1) >= 0) and
      not LogAddress::Done(log).readValue()) // FIXME
    // if (((Log->OldTop - 1) >= 0) and not Log->Done) // FIXME
    // and Log->NewIt not_eq Log->Root)
    State.changeWoEvenOdd(AVLTStateKind::R20, AVLTStateKind::R14);
  else
    State.changeWoEvenOdd(AVLTStateKind::R20, AVLTStateKind::C0);
}

void processStateR21(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR21ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R21, AVLTStateKind::R28);
}

void processStateR22(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR22ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R22, AVLTStateKind::R23);
}

void processStateR23(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR23ToDX(Allocator, log, State);

  // if (Log->Top != 0)
  if (LogAddress::Top(log).readValue() != 0)
    State.changeWoEvenOdd(AVLTStateKind::R23, AVLTStateKind::R15);
  else
    State.changeWoEvenOdd(AVLTStateKind::R23, AVLTStateKind::R16);
}

void processStateR24(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR24ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R24, AVLTStateKind::R25);
}

void processStateR25(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR25ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R25, AVLTStateKind::R26);
}

void processStateR26(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR26ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R26, AVLTStateKind::R27);
}

void processStateR27(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR27ToDX(Allocator, log, State);

  // if (Log->Top != 0)
  if (LogAddress::Top(log).readValue() != 0)
    State.changeWoEvenOdd(AVLTStateKind::R27, AVLTStateKind::R15);
  else
    State.changeWoEvenOdd(AVLTStateKind::R27, AVLTStateKind::R16);
}

void processStateR28(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR28ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R28, AVLTStateKind::R29);
}

void processStateR29(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR29ToDX(Allocator, log, State);

  // if (Log->Top != 0)
  if (LogAddress::Top(log).readValue() != 0)
    State.changeWoEvenOdd(AVLTStateKind::R29, AVLTStateKind::R15);
  else
    State.changeWoEvenOdd(AVLTStateKind::R29, AVLTStateKind::R16);
}

void processStateR30(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR30ToDX(Allocator, log, State);

  // if (Allocator.get(Log->NN)->Balance == 0)
  if (BalanceInNode<LogAddress::NN>(Allocator, {log}).readValue() == 0)
    State.changeWoEvenOdd(AVLTStateKind::R30, AVLTStateKind::R31);
  else if (BalanceInNode<LogAddress::NN>(Allocator, {log}).readValue() ==
           NegateBalance<LogAddress::Balance>(log).readValue())
    // else if (Allocator.get(Log->NN)->Balance == -Log->Balance)
    State.changeWoEvenOdd(AVLTStateKind::R30, AVLTStateKind::R32);
  else /* nn->balance == -bal */
    State.changeWoEvenOdd(AVLTStateKind::R30, AVLTStateKind::R33);
}

void processStateR31(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR31ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R31, AVLTStateKind::R34);
}

void processStateR32(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR32ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R32, AVLTStateKind::R34);
}

void processStateR33(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR33ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R33, AVLTStateKind::R34);
}

void processStateR34(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionR34ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::R34, AVLTStateKind::R24);
}

void processCurrentState(AllocatorType &Allocator, LogType &log,
                         SMStateType &State) {
  AVLTStateKind CurrentState = State.getStateKind();
  switch (CurrentState) {
  case AVLTStateKind::C0:
    assert(false);
    break;
  case AVLTStateKind::A1:
    break;
  case AVLTStateKind::A2:
    break;
  case AVLTStateKind::A3:
    break;
  case AVLTStateKind::A4:
    break;
  case AVLTStateKind::A5:
    break;
  case AVLTStateKind::A6:
    break;
  case AVLTStateKind::A7:
    break;
  case AVLTStateKind::A8:
    break;
  case AVLTStateKind::A9:
    break;
  case AVLTStateKind::A10:
    break;
  case AVLTStateKind::A11:
    break;
  case AVLTStateKind::A12:
    break;
  case AVLTStateKind::A13:
    break;
  case AVLTStateKind::A14:
    break;
  case AVLTStateKind::A15:
    break;
  case AVLTStateKind::A16:
    break;
  case AVLTStateKind::A17:
    break;
  case AVLTStateKind::A18:
    break;
  case AVLTStateKind::A19:
    break;
  case AVLTStateKind::A20:
    break;
  case AVLTStateKind::A21:
    break;
  case AVLTStateKind::A22:
    break;
  case AVLTStateKind::A23:
    break;
  case AVLTStateKind::A24:
    break;
  case AVLTStateKind::A25:
    break;
  case AVLTStateKind::A26:
    break;
  case AVLTStateKind::R1:
    processStateR1(Allocator, log, State);
    break;
  case AVLTStateKind::R2:
    processStateR2(log, State);
    break;
  case AVLTStateKind::R3:
    processStateR3(Allocator, log, State);
    break;
  case AVLTStateKind::R4:
    processStateR4(Allocator, log, State);
    break;
  case AVLTStateKind::R5:
    processStateR5(Allocator, log, State);
    break;
  case AVLTStateKind::R6:
    processStateR6(Allocator, log, State);
    break;
  case AVLTStateKind::R7:
    processStateR7(Allocator, log, State);
    break;
  case AVLTStateKind::R8:
    processStateR8(Allocator, log, State);
    break;
  case AVLTStateKind::R9:
    processStateR9(Allocator, log, State);
    break;
  case AVLTStateKind::R10:
    processStateR10(Allocator, log, State);
    break;
  case AVLTStateKind::R11:
    processStateR11(Allocator, log, State);
    break;
  case AVLTStateKind::R12:
    processStateR12(Allocator, log, State);
    break;
  case AVLTStateKind::R13:
    processStateR13(Allocator, log, State);
    break;
  case AVLTStateKind::R14:
    processStateR14(Allocator, log, State);
    break;
  case AVLTStateKind::R15:
    processStateR15(Allocator, log, State);
    break;
  case AVLTStateKind::R16:
    processStateR16(Allocator, log, State);
    break;
  case AVLTStateKind::R17:
    processStateR17(Allocator, log, State);
    break;
  case AVLTStateKind::R18:
    processStateR18(Allocator, log, State);
    break;
  case AVLTStateKind::R19:
    processStateR19(Allocator, log, State);
    break;
  case AVLTStateKind::R20:
    processStateR20(Allocator, log, State);
    break;
  case AVLTStateKind::R21:
    processStateR21(Allocator, log, State);
    break;
  case AVLTStateKind::R22:
    processStateR22(Allocator, log, State);
    break;
  case AVLTStateKind::R23:
    processStateR23(Allocator, log, State);
    break;
  case AVLTStateKind::R24:
    processStateR24(Allocator, log, State);
    break;
  case AVLTStateKind::R25:
    processStateR25(Allocator, log, State);
    break;
  case AVLTStateKind::R26:
    processStateR26(Allocator, log, State);
    break;
  case AVLTStateKind::R27:
    processStateR27(Allocator, log, State);
    break;
  case AVLTStateKind::R28:
    processStateR28(Allocator, log, State);
    break;
  case AVLTStateKind::R29:
    processStateR29(Allocator, log, State);
    break;
  case AVLTStateKind::R30:
    processStateR30(Allocator, log, State);
    break;
  case AVLTStateKind::R31:
    processStateR31(Allocator, log, State);
    break;
  case AVLTStateKind::R32:
    processStateR32(Allocator, log, State);
    break;
  case AVLTStateKind::R33:
    processStateR33(Allocator, log, State);
    break;
  case AVLTStateKind::R34:
    processStateR34(Allocator, log, State);
    break;
  }
}

} // namespace

namespace Avlt::Remove {

void GoToC0(AllocatorType &Allocator, LogType &log, SMStateType &State,
            uint64_t Key) {
  AVLTStateKind CurrentState = State.getStateKind();

  assert(CurrentState == AVLTStateKind::C0);

  processStateC0(log, State, Key);
  CurrentState = State.getStateKind();

  assert(CurrentState != AVLTStateKind::C0);

  while (CurrentState != AVLTStateKind::C0) {
    processCurrentState(Allocator, log, State);
    CurrentState = State.getStateKind();
  }
}

void RecoverToC0(AllocatorType &Allocator, LogType &log, SMStateType &State) {
  AVLTStateKind CurrentState = State.getStateKind();
  while (CurrentState != AVLTStateKind::C0) {
    processCurrentState(Allocator, log, State);
    CurrentState = State.getStateKind();
  }
}

} // namespace Avlt::Remove
