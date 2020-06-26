#include "state-machine-insert.h"

#include "avlt-allocator.h"
#include "avlt-common.h"
#include "avlt-log.h"
#include "flush.h"
#include "state-kind.h"
#include "tree.h"
#include "write-dsl-avlt.h"
//#include "write-dsl-common.h"
//#include "write-dsl-values.h"

#include <algorithm>
#include <string>

/*
Top-Down Insertion
 */
/*
Donald Knuth's “The Art of Computer Programming”, volume 3
 */

/*
@book{Knuth:1998:ACP:280635,
 author = {Knuth, Donald E.},
 title = {The Art of Computer Programming,  Volume 3: (2Nd Ed.) Sorting and
Searching}, year = {1998}, isbn = {0-201-89685-0}, publisher = {Addison Wesley
Longman Publishing Co., Inc.}, address = {Redwood City, CA, USA},
}
 */
namespace {

// Direction negate(Direction Dir) {
//  if (Dir == Direction::Left)
//    return Direction::Right;
//  if (Dir == Direction::Right)
//    return Direction::Left;
//  assert(false);
//  return Direction::Unknown;
//}

/* flush key and value */
void transitionC0ToDX(LogType &log, SMStateType &State, uint64_t Key,
                      uint64_t Value) {
  // flush(
  //    [&]() {
  //      write(&Log->Key, Key);
  //      write(&Log->Value, Value);
  //    },
  //    FlushKind::Misc);

  WriteOp<LogAddress::Key, KeyValue> W1 = {LogAddress::Key(log), KeyValue(Key)};
  WriteOp<LogAddress::Value, ValueValue> W2 = {LogAddress::Value(log),
                                               ValueValue(Value)};

  flushOp(FlushKind::FlushCommand, W1, W2);
}

/* insert new root node */
void transitionA1ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {

  NodeIndex R = Allocator.getNewNodeFromLog();

  // flush([&]() { write(&Log->Root, R); }, FlushKind::Misc);

  WriteOp<LogAddress::Root, NodePtrValue> W1 = {LogAddress::Root(log),
                                                NodePtrValue(R)};

  flushOp(FlushKind::InsertNewNode, W1);
}

/* init variables */
void transitionA2ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // flush(
  //    [&]() {
  //      write(&Allocator.get(Allocator.getHeadNode())->right, Log->Root);
  //      write(&Log->T, Allocator.getHeadNode());
  //      write(&Log->S, Allocator.get(Allocator.getHeadNode())->right);
  //      write(&Log->P, Allocator.get(Allocator.getHeadNode())->right);
  //    },
  //    FlushKind::Misc);

  NodeIndex Head = Allocator.getHeadNode();

  WriteOp<RightInNode<NodePtrValue>, LogAddress::Root> W1 = {
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Head)),
      LogAddress::Root(log)};

  WriteOp<LogAddress::Tp, NodePtrValue> W2 = {LogAddress::Tp(log),
                                              NodePtrValue(Head)};

  WriteOp<LogAddress::Sp, RightInNode<NodePtrValue>> W3 = {
      LogAddress::Sp(log),
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Head))};

  WriteOp<LogAddress::Pp, RightInNode<NodePtrValue>> W4 = {
      LogAddress::Pp(log),
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(Head))};

  flushOp(FlushKind::InitializeVariables, W1, W2, W3, W4);
}
/* header of first for loop */
void transitionA3ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  Direction Dir = Direction::Unknown;
  if (SmallerThanKey<KeyInNode<LogAddress::Pp>, LogAddress::Key>(
          KeyInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log)),
          LogAddress::Key(log))
          .readValue())
    // if (Allocator.get(Log->P)->key < Log->Key)
    Dir = Direction::Right;
  else
    Dir = Direction::Left;

  NodeIndex NextQ = Nullptr;

  // assert(Log->P != Nullptr);

  if (Dir == Direction::Right)
    // NextQ = Allocator.get(Log->P)->right;
    NextQ =
        RightInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log)).readValue();
  else
    NextQ =
        LeftInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log)).readValue();
  // NextQ = Allocator.get(Log->P)->left;

  // flush(
  //    [&]() {
  //      write(&Log->Q, NextQ);
  //      write(&Log->Dir, Dir);
  //    },
  //    FlushKind::Misc);

  WriteOp<LogAddress::QP, NodePtrValue> W1 = {LogAddress::QP(log),
                                              NodePtrValue(NextQ)};
  WriteOp<LogAddress::Dir, DirectionValue> W2 = {LogAddress::Dir(log),
                                                 DirectionValue(Dir)};

  flushOp(FlushKind::Loop, W1, W2);
}

/* update balance points */
void transitionA5ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // flush(
  //    [&]() {
  //      write(&Log->T, Log->P);
  //      write(&Log->S, Log->Q);
  //    },
  //    FlushKind::Misc);

  WriteOp<LogAddress::Tp, LogAddress::Pp> W1 = {LogAddress::Tp(log),
                                                LogAddress::Pp(log)};
  WriteOp<LogAddress::Sp, LogAddress::QP> W2 = {LogAddress::Sp(log),
                                                LogAddress::QP(log)};

  flushOp(FlushKind::BalancePoints, W1, W2);
}

/* increment for first loop */
void transitionA6ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {

  // flush([&]() { write(&Log->P, Log->Q); }, FlushKind::Misc);

  WriteOp<LogAddress::Pp, LogAddress::QP> W1 = {LogAddress::Pp(log),
                                                LogAddress::QP(log)};

  flushOp(FlushKind::Loop, W1);
}

/* header of first for loop  with look ahead */
void transitionA25ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  NodeIndex P = LogAddress::Pp(log).readValue(); // Log->P;
  NodeIndex S = LogAddress::Sp(log).readValue(); // Log->S;
  NodeIndex T = LogAddress::Tp(log).readValue(); // Log->T;
  NodeIndex NextQ = Nullptr;

  Direction Dir = Direction::Unknown;

  do {
    if (SmallerThanKey<KeyInNode<NodePtrValue>, LogAddress::Key>(
            KeyInNode<NodePtrValue>(Allocator, NodePtrValue(P)),
            LogAddress::Key(log))
            .readValue())
      // if (Allocator.get(P)->key < Log->Key)
      Dir = Direction::Right;
    else
      Dir = Direction::Left;

    if (Dir == Direction::Right)
      // NextQ = Allocator.get(P)->right;
      NextQ = RightInNode<NodePtrValue>(Allocator, NodePtrValue(P)).readValue();
    else
      NextQ = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(P)).readValue();
    // NextQ = Allocator.get(P)->left;

    //printf("NextQ %u\n", NextQ.getValue());
    if (NextQ == Nullptr)
      break;

    if (BalanceInNode<NodePtrValue>(Allocator, NodePtrValue(NextQ))
            .readValue() != 0) {
      // if (Allocator.get(NextQ)->Balance != 0) {
      S = NextQ;
      T = P;
    }

    P = NextQ;

  } while (true);

  // flush(
  //    [&]() {
  //      write(&Log->NextP, P);
  //      write(&Log->Dir, Dir);
  //      write(&Log->NextS, S);
  //      write(&Log->NextT, T);
  //    },
  //    FlushKind::Misc);

  WriteOp<LogAddress::NextP, NodePtrValue> W1 = {LogAddress::NextP(log),
                                                 NodePtrValue(P)};

  WriteOp<LogAddress::Dir, DirectionValue> W2 = {LogAddress::Dir(log),
                                                 DirectionValue(Dir)};

  WriteOp<LogAddress::NextS, NodePtrValue> W3 = {LogAddress::NextS(log),
                                                 NodePtrValue(S)};

  WriteOp<LogAddress::NextT, NodePtrValue> W4 = {LogAddress::NextT(log),
                                                 NodePtrValue(T)};

  flushOp(FlushKind::Loop, W1, W2, W3, W4);
}

/* cleanup of look ahead */
void transitionA26ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  // flush(
  //    [&]() {
  //      write(&Log->P, Log->NextP);
  //      write(&Log->S, Log->NextS);
  //      write(&Log->T, Log->NextT);
  //    },
  //    FlushKind::Misc);

  WriteOp<LogAddress::Pp, LogAddress::NextP> W1 = {LogAddress::Pp(log),
                                                   LogAddress::NextP(log)};
  WriteOp<LogAddress::Sp, LogAddress::NextS> W2 = {LogAddress::Sp(log),
                                                   LogAddress::NextS(log)};
  WriteOp<LogAddress::Tp, LogAddress::NextT> W3 = {LogAddress::Tp(log),
                                                   LogAddress::NextT(log)};

  flushOp(FlushKind::Loop, W1, W2, W3);
}

/* Insert the new node */
void transitionA4ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  NodeIndex R = Allocator.getNewNodeFromLog();

  // printf("A4: q=%d\n", Log->P.getValue());

  WriteOp<LogAddress::QP, NodePtrValue> W1 = {LogAddress::QP(log),
                                              NodePtrValue(R)};

  if (LogAddress::Dir(log).readValue() == Direction::Left) { // Log->Dir
    WriteOp<LeftInNode<LogAddress::Pp>, NodePtrValue> W2 = {
        LeftInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log)),
        NodePtrValue(R)};

    flushOp(FlushKind::InsertNewNode, W1, W2);

    // flush(
    //    [&]() {
    //      write(&Log->Q, R);
    //      write(&Allocator.get(Log->P)->left, R);
    //    },
    //    FlushKind::Misc);
  } else {
    WriteOp<RightInNode<LogAddress::Pp>, NodePtrValue> W2 = {
        RightInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log)),
        NodePtrValue(R)};

    flushOp(FlushKind::InsertNewNode, W1, W2);
    // flush(
    //    [&]() {
    //      write(&Log->Q, R);
    //      write(&Allocator.get(Log->P)->right, R);
    //    },
    //    FlushKind::Misc);
  }
}

/* update balance factors init */
void transitionA7ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // printf("A7: s=%d, q=%d\n", Log->S.getValue(), Log->Q.getValue());

  WriteOp<LogAddress::Pp, LogAddress::Sp> W1 = {LogAddress::Pp(log),
                                                LogAddress::Sp(log)};

  flushOp(FlushKind::BalanceFactors, W1);

  // flush([&]() { write(&Log->P, Log->S); }, FlushKind::Misc);
}

/* update balance factors body log */
void transitionA8ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // assert(Log->P != Nullptr);

  Direction Dir = Direction::Unknown;
  if (SmallerThanKey<KeyInNode<LogAddress::Pp>, LogAddress::Key>(
          KeyInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log)),
          LogAddress::Key(log))
          .readValue())
    // if (Allocator.get(Log->P)->key < Log->Key)
    Dir = Direction::Right;
  else
    Dir = Direction::Left;

  int8_t Inc = 0;
  if (Dir == Direction::Left)
    Inc = -1;
  else
    Inc = +1;

  WriteOp<LogAddress::Dir, DirectionValue> W1 = {LogAddress::Dir(log),
                                                 DirectionValue(Dir)};
  WriteOp<LogAddress::Inc, Int8Value> W2 = {LogAddress::Inc(log),
                                            Int8Value(Inc)};
  WriteOp<LogAddress::Balance2, BalanceInNode<LogAddress::Pp>> W3 = {
      LogAddress::Balance2(log),
      BalanceInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log))};

  flushOp(FlushKind::BalanceFactors, W1, W2, W3);

  // flush(
  //    [&]() {
  //      write(&Log->Dir, Dir);
  //      write(&Log->Inc, Inc);
  //      write(&Log->Balance2, Allocator.get(Log->P)->Balance);
  //    },
  //    FlushKind::Misc);
}

/* update balance factors increment log */
void transitionA9ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  NodeIndex NextP = Nullptr;
  if (LogAddress::Dir(log).readValue() == Direction::Left)
    // if (Log->Dir == Direction::Left)
    NextP = LeftInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log))
                .readValue(); // Allocator.get(Log->P)->left;
  else
    NextP = RightInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log))
                .readValue(); // Allocator.get(Log->P)->right;
  // NextP = Allocator.get(Log->P)->right;

  // plotNode(Allocator, Log->P);

  // assert(NextP != Nullptr);

  WriteOp<LogAddress::NextP, NodePtrValue> W1 = {LogAddress::NextP(log),
                                                 NodePtrValue(NextP)};

  flushOp(FlushKind::BalanceFactors, W1);
  // flush([&]() { write(&Log->NextP, NextP); }, FlushKind::Misc);
}

/* post update balance factors loop */
/* save rebalance point for parent fix */
void transitionA10ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::QP, LogAddress::Sp> W1 = {LogAddress::QP(log),
                                                LogAddress::Sp(log)};

  flushOp(FlushKind::BalancePoints, W1);
  // flush([&]() { write(&Log->Q, Log->S); }, FlushKind::Misc);
}

/* update balance factors body update */
void transitionA11ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  int8_t NewBalance = static_cast<int8_t>(
      LogAddress::Balance2(log).readValue() + LogAddress::Inc(log).readValue());

  WriteOp<BalanceInNode<LogAddress::Pp>, Int8Value> W1 = {
      BalanceInNode<LogAddress::Pp>(Allocator, LogAddress::Pp(log)),
      Int8Value(NewBalance)};

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Log->P)->Balance,
  //            static_cast<int8_t>(Log->Balance2 + Log->Inc));
  //    },
  //    FlushKind::Misc);

  flushOp(FlushKind::BalanceFactors, W1);
}

/* update balance factors increment update */
void transitionA12ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::Pp, LogAddress::NextP> W1 = {LogAddress::Pp(log),
                                                   LogAddress::NextP(log)};

  flushOp(FlushKind::BalanceFactors, W1);
  // flush([&]() { write(&Log->P, Log->NextP); }, FlushKind::Misc);
}

/* rebalance if necessary log and logging for/in insert_balance */
void transitionA13ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // root=s dir=dir

  Direction Dir = Direction::Unknown;
  if (SmallerThanKey<KeyInNode<LogAddress::Sp>, LogAddress::Key>(
          KeyInNode<LogAddress::Sp>(Allocator, LogAddress::Sp(log)),
          LogAddress::Key(log))
          .readValue())
    // if (Allocator.get(Log->S)->key < Log->Key)
    Dir = Direction::Right;
  else
    Dir = Direction::Left;

  int8_t Balance = 0;

  if (Dir == Direction::Left)
    Balance = -1;
  else
    Balance = +1;

  NodeIndex NN = Nullptr;
  if (Dir == Direction::Left)
    // NN = Allocator.get(Log->S)->left;
    NN = LeftInNode<LogAddress::Sp>(Allocator, {log}).readValue();
  else
    NN = RightInNode<LogAddress::Sp>(Allocator, {log}).readValue();
  // NN = Allocator.get(Log->S)->right;

  int8_t NBalance =
      BalanceInNode<NodePtrValue>(Allocator, NodePtrValue(NN)).readValue();
  // int8_t NBalance = Allocator.get(NN)->Balance;

  WriteOp<LogAddress::Dir, DirectionValue> W1 = {{log}, DirectionValue(Dir)};
  WriteOp<LogAddress::SPp, LogAddress::Sp> W2 = {{log}, {log}};
  WriteOp<LogAddress::Balance, Int8Value> W3 = {{log}, Int8Value(Balance)};
  WriteOp<LogAddress::N, NodePtrValue> W4 = {{log}, NodePtrValue(NN)};
  WriteOp<LogAddress::Balance2, Int8Value> W5 = {{log}, Int8Value(NBalance)};

  flushOp(FlushKind::Balance, W1, W2, W3, W4, W5);

  // flush(
  //    [&]() {
  //      write(&Log->Dir, Dir);
  //      write(&Log->SP, Log->S);
  //      write(&Log->Balance, Balance);
  //      write(&Log->N, NN);
  //      write(&Log->Balance2, NBalance);
  //    },
  //    FlushKind::Misc);
}

/* logging for single rotation*/
void transitionA14ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  NodeIndex Save = Nullptr;
  NodeIndex SaveChild = Nullptr;

  // Root is S and !Dir
  if (NegateDirection<LogAddress::Dir>(log).readValue() == Direction::Left)
    // if (negate(Log->Dir) == Direction::Left)
    Save = RightInNode<LogAddress::Sp>(Allocator, {log}).readValue();
  // Save = Allocator.get(Log->S)->right;
  else
    Save = LeftInNode<LogAddress::Sp>(Allocator, {log}).readValue();
  // Save = Allocator.get(Log->S)->left;

  if (NegateDirection<LogAddress::Dir>(log).readValue() == Direction::Left)
    // if (negate(Log->Dir) == Direction::Left)
    SaveChild =
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
  // SaveChild = Allocator.get(Save)->left;
  else
    SaveChild =
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
  // SaveChild = Allocator.get(Save)->right;

  // printf("A14: root=%u save=%u child=%u\n", Log->S.getValue(),
  //        Save.getValue(), SaveChild.getValue());

  //  if (SaveChild == Nullptr)
  //    printf("single rotation: root=%llu save=%llu child=%u\n",
  //           Allocator.get(Log->S)->key, Allocator.get(Save)->key,
  //           Nullptr.getValue());
  //  else
  //    printf("single rotation: root=%llu save=%llu child=%llu\n",
  //           Allocator.get(Log->S)->key, Allocator.get(Save)->key,
  //           Allocator.get(SaveChild)->key);

  WriteOp<BalanceInNode<LogAddress::Sp>, Int8Value> W1 = {{Allocator, {log}},
                                                          Int8Value(0)};
  WriteOp<BalanceInNode<LogAddress::N>, Int8Value> W2 = {{Allocator, {log}},
                                                         Int8Value(0)};
  WriteOp<LogAddress::SaveP, NodePtrValue> W3 = {{log}, NodePtrValue(Save)};
  WriteOp<LogAddress::SaveChildP, NodePtrValue> W4 = {{log},
                                                      NodePtrValue(SaveChild)};
  WriteOp<LogAddress::SPp, LogAddress::Sp> W5 = {{log}, {log}};

  flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5);

  // flush(
  //    [&]() {
  //      write(&Allocator.get(Log->S)->Balance, static_cast<int8_t>(0));
  //      write(&Allocator.get(Log->N)->Balance, static_cast<int8_t>(0));
  //      write(&Log->Save, Save);
  //      write(&Log->SaveChild, SaveChild);
  //      write(&Log->SP, Log->S);
  //    },
  //    FlushKind::Misc);
}

/* single rotation*/
void transitionA15ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // printf("single rotation\n");

  // FIXME: order-dependent
  if (NegateDirection<LogAddress::Dir>(log).readValue() == Direction::Left) {
    // if (negate(Log->Dir) == Direction::Left) {
    WriteOp<RightInNode<LogAddress::Sp>, LogAddress::SaveChildP> W1 = {
        {Allocator, {log}}, {log}};
    WriteOp<LeftInNode<LogAddress::SaveP>, LogAddress::SPp> W2 = {
        {Allocator, {log}}, {log}};
    WriteOp<LogAddress::Sp, LogAddress::SaveP> W3 = {{log}, {log}};

    flushOp(FlushKind::SingleRotation, W1, W2, W3);

    // flush(
    //    [&]() {
    //      write(&Allocator.get(Log->SP)->right, Log->SaveChild);
    //      write(&Allocator.get(Log->Save)->left, Log->SP);
    //      write(&Log->S, Log->Save);
    //    },
    //    FlushKind::Misc);
  } else {

    WriteOp<LeftInNode<LogAddress::Sp>, LogAddress::SaveChildP> W1 = {
        {Allocator, {log}}, {log}};
    WriteOp<RightInNode<LogAddress::SaveP>, LogAddress::SPp> W2 = {
        {Allocator, {log}}, {log}};
    WriteOp<LogAddress::Sp, LogAddress::SaveP> W3 = {{log}, {log}};

    flushOp(FlushKind::SingleRotation, W1, W2, W3);

    // flush(
    //    [&]() {
    //      write(&Allocator.get(Log->SP)->left, Log->SaveChild);
    //      write(&Allocator.get(Log->Save)->right, Log->SP);
    //      write(&Log->S, Log->Save);
    //    },
    //    FlushKind::Misc);
  }
} // namespace

/* logging for adjust balance */
void transitionA16ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  NodeIndex N = Nullptr;
  NodeIndex NN = Nullptr;
  int8_t NNBalance = 0;

  if (LogAddress::Dir(log).readValue() == Direction::Left) {
    // if (Log->Dir == Direction::Left) {
    N = LeftInNode<LogAddress::Sp>(Allocator, {log}).readValue();
    NN = RightInNode<NodePtrValue>(Allocator, NodePtrValue(N)).readValue();
    // N = Allocator.get(Log->S)->left;
    // NN = Allocator.get(N)->right;
  } else {
    N = RightInNode<LogAddress::Sp>(Allocator, {log}).readValue();
    NN = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(N)).readValue();
    // N = Allocator.get(Log->S)->right;
    // NN = Allocator.get(N)->left;
  }

  NNBalance =
      BalanceInNode<NodePtrValue>(Allocator, NodePtrValue(NN)).readValue();
  // NNBalance = Allocator.get(NN)->Balance;

  WriteOp<LogAddress::N, NodePtrValue> W1 = {{log}, NodePtrValue(N)};
  WriteOp<LogAddress::NN, NodePtrValue> W2 = {{log}, NodePtrValue(NN)};
  WriteOp<LogAddress::Balance3, Int8Value> W3 = {{log}, Int8Value(NNBalance)};

  flushOp(FlushKind::Balance, W1, W2, W3);

  // flush(
  //    [&]() {
  //      write(&Log->N, N);
  //      write(&Log->NN, NN);
  //      write(&Log->Balance3, NNBalance);
  //    },
  //    FlushKind::Misc);
}

/* adjust balance */
void transitionA17ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  if (LogAddress::Balance3(log).readValue() == 0) {
    // if (Log->Balance3 == 0)
    WriteOp<BalanceInNode<LogAddress::Sp>, Int8Value> W1 = {{Allocator, {log}},
                                                            Int8Value(0)};
    WriteOp<BalanceInNode<LogAddress::N>, Int8Value> W2 = {{Allocator, {log}},
                                                           Int8Value(0)};
    WriteOp<BalanceInNode<LogAddress::NN>, Int8Value> W3 = {{Allocator, {log}},
                                                            Int8Value(0)};

    flushOp(FlushKind::Balance, W1, W2, W3);

    // flush(
    //    [&]() {
    //      write(&Allocator.get(Log->S)->Balance, static_cast<int8_t>(0));
    //      write(&Allocator.get(Log->N)->Balance, static_cast<int8_t>(0));
    //      write(&Allocator.get(Log->NN)->Balance, static_cast<int8_t>(0));
    //    },
    //    FlushKind::Misc);
  } else if (LogAddress::Balance3(log).readValue() ==
             LogAddress::Balance(log).readValue()) {

    WriteOp<BalanceInNode<LogAddress::Sp>, NegateBalance<LogAddress::Balance>>
        W1 = {{Allocator, {log}}, {log}};
    WriteOp<BalanceInNode<LogAddress::N>, Int8Value> W2 = {{Allocator, {log}},
                                                           Int8Value(0)};
    WriteOp<BalanceInNode<LogAddress::NN>, Int8Value> W3 = {{Allocator, {log}},
                                                            Int8Value(0)};

    flushOp(FlushKind::Balance, W1, W2, W3);

    // flush(
    //    [&]() {
    //      write(&Allocator.get(Log->S)->Balance,
    //            static_cast<int8_t>(-Log->Balance));
    //      write(&Allocator.get(Log->N)->Balance, static_cast<int8_t>(0));
    //      write(&Allocator.get(Log->NN)->Balance, static_cast<int8_t>(0));
    //    },
    //    FlushKind::Misc);
  } else {
    WriteOp<BalanceInNode<LogAddress::Sp>, Int8Value> W1 = {{Allocator, {log}},
                                                            Int8Value(0)};
    WriteOp<BalanceInNode<LogAddress::N>, LogAddress::Balance> W2 = {
        {Allocator, {log}}, {log}};
    WriteOp<BalanceInNode<LogAddress::NN>, Int8Value> W3 = {{Allocator, {log}},
                                                            Int8Value(0)};
    flushOp(FlushKind::Balance, W1, W2, W3);

    // flush(
    //    [&]() {
    //      write(&Allocator.get(Log->S)->Balance, static_cast<int8_t>(0));
    //      write(&Allocator.get(Log->N)->Balance, Log->Balance);
    //      write(&Allocator.get(Log->NN)->Balance, static_cast<int8_t>(0));
    //    },
    //    FlushKind::Misc);
  }
} // namespace

/* logging for first single in double roation */
void transitionA18ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // Dir root=Log->S->link[!Dir]

  NodeIndex Root = Nullptr;
  NodeIndex Save = Nullptr;
  NodeIndex SaveChild = Nullptr;

  if (LogAddress::Dir(log).readValue() == Direction::Left)
    // if (Log->Dir == Direction::Left)
    Root = LeftInNode<LogAddress::Sp>(Allocator, {log}).readValue();
  // Root = Allocator.get(Log->S)->left;
  else
    Root = RightInNode<LogAddress::Sp>(Allocator, {log}).readValue();
  // Root = Allocator.get(Log->S)->right;

  assert(Root != Nullptr);

  if (LogAddress::Dir(log).readValue() == Direction::Left) {
    // if (Log->Dir == Direction::Left) {
    Save = RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
    // Save = Allocator.get(Root)->right;
    assert(Save != Nullptr);
    SaveChild =
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
    // SaveChild = Allocator.get(Save)->left;
  } else {
    Save = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
    SaveChild =
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
    // Save = Allocator.get(Root)->left;
    // assert(Save != Nullptr);
    // SaveChild = Allocator.get(Save)->right;
  }

  //  if (SaveChild == Nullptr)
  //    printf("first single in double rotation: root=%llu save=%llu
  //    child=%u\n",
  //           Allocator.get(Root)->key, Allocator.get(Save)->key,
  //           Nullptr.getValue());
  //  else
  //    printf("first single in double rotation: root=%llu save=%llu
  //    child=%llu\n",
  //           Allocator.get(Root)->key, Allocator.get(Save)->key,
  //           Allocator.get(SaveChild)->key);

  WriteOp<LogAddress::SaveP, NodePtrValue> W1 = {{log}, NodePtrValue(Save)};
  WriteOp<LogAddress::SaveChildP, NodePtrValue> W2 = {{log},
                                                      NodePtrValue(SaveChild)};
  WriteOp<LogAddress::Root2P, NodePtrValue> W3 = {{log}, NodePtrValue(Root)};

  flushOp(FlushKind::DoubleRotation, W1, W2, W3);

  // flush(
  //    [&]() {
  //      write(&Log->Save, Save);
  //      write(&Log->SaveChild, SaveChild);
  //      write(&Log->Root2, Root);
  //    },
  //    FlushKind::Misc);
}

/* first single in double roation */
void transitionA19ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  // Dir root=Log->S

  if (LogAddress::Dir(log).readValue() == Direction::Left) {
    // if (Log->Dir == Direction::Left) {
    WriteOp<RightInNode<LogAddress::Root2P>, LogAddress::SaveChildP> W1 = {
        {Allocator, {log}}, {log}};
    WriteOp<LeftInNode<LogAddress::SaveP>, LogAddress::Root2P> W2 = {
        {Allocator, {log}}, {log}};
    WriteOp<LeftInNode<LogAddress::Sp>, LogAddress::SaveP> W3 = {
        {Allocator, {log}}, {log}};

    flushOp(FlushKind::DoubleRotation, W1, W2, W3);
    // flush(
    //    [&]() {
    //      write(&Allocator.get(Log->Root2)->right, Log->SaveChild);
    //      write(&Allocator.get(Log->Save)->left, Log->Root2);
    //      write(&Allocator.get(Log->S)->left, Log->Save);
    //    },
    //    FlushKind::Misc);
  } else {

    WriteOp<LeftInNode<LogAddress::Root2P>, LogAddress::SaveChildP> W1 = {
        {Allocator, {log}}, {log}};
    WriteOp<RightInNode<LogAddress::SaveP>, LogAddress::Root2P> W2 = {
        {Allocator, {log}}, {log}};
    WriteOp<RightInNode<LogAddress::Sp>, LogAddress::SaveP> W3 = {
        {Allocator, {log}}, {log}};

    flushOp(FlushKind::DoubleRotation, W1, W2, W3);
    // flush(
    //    [&]() {
    //      write(&Allocator.get(Log->Root2)->left, Log->SaveChild);
    //      write(&Allocator.get(Log->Save)->right, Log->Root2);
    //      write(&Allocator.get(Log->S)->right, Log->Save);
    //    },
    //    FlushKind::Misc);
  }
}

/* logging for second single in double roation */
void transitionA20ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // !Dir Log->S

  NodeIndex Root = LogAddress::Sp(log).readValue(); // Log->S;
  NodeIndex Save = Nullptr;
  NodeIndex SaveChild = Nullptr;

  if (NegateDirection<LogAddress::Dir>(log).readValue() == Direction::Left) {
    // if (negate(Log->Dir) == Direction::Left) {
    Save = RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
    // Save = Allocator.get(Root)->right;
    assert(Save != Nullptr);
    SaveChild =
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
    // SaveChild = Allocator.get(Save)->left;
  } else {
    Save = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)).readValue();
    // Save = Allocator.get(Root)->left;
    assert(Save != Nullptr);
    SaveChild =
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
    // SaveChild = Allocator.get(Save)->right;
  }

  //  if (SaveChild == Nullptr)
  //    printf("second single in double rotation: root=%llu save=%llu
  //    child=%u\n",
  //           Allocator.get(Root)->key, Allocator.get(Save)->key,
  //           Nullptr.getValue());
  //  else
  //    printf("second single in double rotation: root=%llu save=%llu
  //    child=%llu\n",
  //           Allocator.get(Root)->key, Allocator.get(Save)->key,
  //           Allocator.get(SaveChild)->key);

  WriteOp<LogAddress::Root2P, NodePtrValue> W1 = {{log}, NodePtrValue(Root)};
  WriteOp<LogAddress::SaveP, NodePtrValue> W2 = {{log}, NodePtrValue(Save)};
  WriteOp<LogAddress::SaveChildP, NodePtrValue> W3 = {{log},
                                                      NodePtrValue(SaveChild)};

  flushOp(FlushKind::DoubleRotation, W1, W2, W3);

  // flush(
  //    [&]() {
  //      write(&Log->Root2, Root);
  //      write(&Log->Save, Save);
  //      write(&Log->SaveChild, SaveChild);
  //    },
  //    FlushKind::Misc);
}

/* second single in double roation */
void transitionA21ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  if (NegateDirection<LogAddress::Dir>(log).readValue() == Direction::Left) {
    // if (negate(Log->Dir) == Direction::Left) {

    WriteOp<RightInNode<LogAddress::Root2P>, LogAddress::SaveChildP> W1 = {
        {Allocator, {log}}, {log}};
    WriteOp<LeftInNode<LogAddress::SaveP>, LogAddress::Root2P> W2 = {
        {Allocator, {log}}, {log}};
    WriteOp<LogAddress::Sp, LogAddress::SaveP> W3 = {{log}, {log}};

    flushOp(FlushKind::DoubleRotation, W1, W2, W3);

    // flush(
    //    [&]() {
    //      write(&Allocator.get(Log->Root2)->right, Log->SaveChild);
    //      write(&Allocator.get(Log->Save)->left, Log->Root2);
    //      write(&Log->S, Log->Save);
    //    },
    //    FlushKind::Misc);
  } else {

    WriteOp<LeftInNode<LogAddress::Root2P>, LogAddress::SaveChildP> W1 = {
        {Allocator, {log}}, {log}};
    WriteOp<RightInNode<LogAddress::SaveP>, LogAddress::Root2P> W2 = {
        {Allocator, {log}}, {log}};
    WriteOp<LogAddress::Sp, LogAddress::SaveP> W3 = {{log}, {log}};

    flushOp(FlushKind::DoubleRotation, W1, W2, W3);

    // flush(
    //    [&]() {
    //      write(&Allocator.get(Log->Root2)->left, Log->SaveChild);
    //      write(&Allocator.get(Log->Save)->right, Log->Root2);
    //      write(&Log->S, Log->Save);
    //    },
    //    FlushKind::Misc);
  }
}

/* fix parent first variant */
void transitionA22ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  WriteOp<LogAddress::Root, LogAddress::Sp> W1 = {{log}, {log}};

  flushOp(FlushKind::FixParent, W1);

  // flush([&]() { write(&Log->Root, Log->S); }, FlushKind::Misc);
}

/* logging fix parent second variant */
void transitionA23ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  Direction Dir = Direction::Unknown;

  // printf("A23: q=%u t=%u s=%u\n", Log->Q.getValue(), Log->T.getValue(),
  // Log->S.getValue());

  if (LogAddress::QP(log).readValue() ==
      RightInNode<LogAddress::Tp>(Allocator, {log}).readValue())
    // if (Log->Q == Allocator.get(Log->T)->right)
    Dir = Direction::Right;
  else
    Dir = Direction::Left;

  // flush([&]() { write(&Log->Dir, Dir); }, FlushKind::Misc);

  WriteOp<LogAddress::Dir, DirectionValue> W1 = {{log}, DirectionValue(Dir)};

  flushOp(FlushKind::FixParent, W1);
}

/* execute fix parent second variant */
void transitionA24ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();

  // Direction Dir = Log->Dir;

  Direction Dir = LogAddress::Dir(log).readValue();

  // printf("A24: T=%u S=%u dir=%s\n", Log->T.getValue(), Log->S.getValue(),
  //       Direction2String(Dir).c_str());

  if (Dir == Direction::Left) {
    WriteOp<LeftInNode<LogAddress::Tp>, LogAddress::Sp> W1 = {
        {Allocator, {log}}, {log}};

    flushOp(FlushKind::FixParent, W1);

    // flush([&]() { write(&Allocator.get(Log->T)->left, Log->S); },
    //      FlushKind::Misc);
  } else {

    WriteOp<RightInNode<LogAddress::Tp>, LogAddress::Sp> W1 = {
        {Allocator, {log}}, {log}};

    flushOp(FlushKind::FixParent, W1);

    // flush([&]() { write(&Allocator.get(Log->T)->right, Log->S); },
    //      FlushKind::Misc);
  }
}

void processStateC0(LogType &log, SMStateType &State, uint64_t Key,
                    uint64_t Value) {
  // AVLTLogStructure *Log = log.getLog();
  transitionC0ToDX(log, State, Key, Value);

  if (IsNullPtr<LogAddress::Root>(log).readValue())
    // if (Log->Root == Nullptr)
    State.changeWoEvenOdd(AVLTStateKind::C0, AVLTStateKind::A1);
  else
    State.changeWoEvenOdd(AVLTStateKind::C0, AVLTStateKind::A2);
}

void processStateA1(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA1ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A1, AVLTStateKind::C0);
}

void processStateA2(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA2ToDX(Allocator, log, State);

  //State.changeWoEvenOdd(AVLTStateKind::A2, AVLTStateKind::A3);
  State.changeWoEvenOdd(AVLTStateKind::A2, AVLTStateKind::A25);
}

void processStateA3(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // AVLTLogStructure *Log = log.getLog();
  transitionA3ToDX(Allocator, log, State);

  // printf("Q: %u\n", Log->Q.getValue());

  if (IsNullPtr<LogAddress::QP>(log).readValue())
    // if (Log->Q == Nullptr)
    State.changeWoEvenOdd(AVLTStateKind::A3, AVLTStateKind::A4);
  // else if (Allocator.get(Log->Q)->Balance != 0)
  else if (BalanceInNode<LogAddress::QP>(Allocator, {log}).readValue() != 0)
    State.changeWoEvenOdd(AVLTStateKind::A3, AVLTStateKind::A5);
  else
    State.changeWoEvenOdd(AVLTStateKind::A3, AVLTStateKind::A6);
}

void processStateA4(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA4ToDX(Allocator, log, State);

  // AVLTLogStructure *Log = log.getLog();
  if (IsNullPtr<LogAddress::QP>(log).readValue())
    // if (Log->Q == Nullptr)
    State.changeWoEvenOdd(AVLTStateKind::A4, AVLTStateKind::C0);
  else
    State.changeWoEvenOdd(AVLTStateKind::A4, AVLTStateKind::A7);
}

void processStateA5(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA5ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A5, AVLTStateKind::A6);
}

void processStateA6(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA6ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A6, AVLTStateKind::A3);
}

void processStateA7(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA7ToDX(Allocator, log, State);

  // AVLTLogStructure *Log = log.getLog();
  if (not EqualNodes<LogAddress::Pp, LogAddress::QP>({log}, {log}).readValue())
    // if (Log->P != Log->Q)
    State.changeWoEvenOdd(AVLTStateKind::A7, AVLTStateKind::A8);
  else
    State.changeWoEvenOdd(AVLTStateKind::A7, AVLTStateKind::A10);
}

void processStateA8(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA8ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A8, AVLTStateKind::A11);
}

void processStateA9(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA9ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A9, AVLTStateKind::A12);
}

void processStateA10(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA10ToDX(Allocator, log, State);

  // AVLTLogStructure *Log = log.getLog();

  NodeIndex Head = Allocator.getHeadNode();
  if (std::abs(BalanceInNode<LogAddress::Sp>(Allocator, {log}).readValue()) > 1)

    // if (std::abs(Allocator.get(Log->S)->Balance) > 1)
    State.changeWoEvenOdd(AVLTStateKind::A10, AVLTStateKind::A13);
  else if (EqualNodes<LogAddress::QP, RightInNode<NodePtrValue>>(
               {log}, {Allocator, NodePtrValue(Head)})
               .readValue())
    // else if (Log->Q == Allocator.get(Allocator.getHeadNode())->right)
    State.changeWoEvenOdd(AVLTStateKind::A10, AVLTStateKind::A22);
  else
    State.changeWoEvenOdd(AVLTStateKind::A10, AVLTStateKind::A23);
}

void processStateA11(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA11ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A11, AVLTStateKind::A9);
}

void processStateA12(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA12ToDX(Allocator, log, State);

  // AVLTLogStructure *Log = log.getLog();

  if (not EqualNodes<LogAddress::Pp, LogAddress::QP>({log}, {log}).readValue())
    // if (Log->P != Log->Q)
    State.changeWoEvenOdd(AVLTStateKind::A12, AVLTStateKind::A8);
  else
    State.changeWoEvenOdd(AVLTStateKind::A12, AVLTStateKind::A10);
}

void processStateA13(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA13ToDX(Allocator, log, State);

  // AVLTLogStructure *Log = log.getLog();

  if (LogAddress::Balance2(log).readValue() ==
      LogAddress::Balance(log).readValue())
    // if (Log->Balance2 == Log->Balance)
    State.changeWoEvenOdd(AVLTStateKind::A13, AVLTStateKind::A14);
  else
    State.changeWoEvenOdd(AVLTStateKind::A13, AVLTStateKind::A16);
}

void processStateA14(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA14ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A14, AVLTStateKind::A15);
}

void processStateA15(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA15ToDX(Allocator, log, State);

  // AVLTLogStructure *Log = log.getLog();

  NodeIndex Head = Allocator.getHeadNode();
  if (EqualNodes<LogAddress::QP, RightInNode<NodePtrValue>>(
          {log}, {Allocator, NodePtrValue(Head)})
          .readValue())
    // if (Log->Q == Allocator.get(Allocator.getHeadNode())->right)
    State.changeWoEvenOdd(AVLTStateKind::A15, AVLTStateKind::A22);
  else
    State.changeWoEvenOdd(AVLTStateKind::A15, AVLTStateKind::A23);
}

void processStateA16(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA16ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A16, AVLTStateKind::A17);
}

void processStateA17(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA17ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A17, AVLTStateKind::A18);
}

void processStateA18(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA18ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A18, AVLTStateKind::A19);
}

void processStateA19(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA19ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A19, AVLTStateKind::A20);
}

void processStateA20(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA20ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A20, AVLTStateKind::A21);
}

void processStateA21(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA21ToDX(Allocator, log, State);

  // AVLTLogStructure *Log = log.getLog();

  NodeIndex Head = Allocator.getHeadNode();
  if (EqualNodes<LogAddress::QP, RightInNode<NodePtrValue>>(
          {log}, {Allocator, NodePtrValue(Head)})
          .readValue())

    // if (Log->Q == Allocator.get(Allocator.getHeadNode())->right)
    State.changeWoEvenOdd(AVLTStateKind::A21, AVLTStateKind::A22);
  else
    State.changeWoEvenOdd(AVLTStateKind::A21, AVLTStateKind::A23);
}

void processStateA22(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA22ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A22, AVLTStateKind::C0);
}

void processStateA23(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA23ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A23, AVLTStateKind::A24);
}

void processStateA24(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA24ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A24, AVLTStateKind::C0);
}

void processStateA25(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA25ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A25, AVLTStateKind::A26);
}

void processStateA26(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA26ToDX(Allocator, log, State);

  State.changeWoEvenOdd(AVLTStateKind::A26, AVLTStateKind::A4);
}

void processCurrentState(AllocatorType &Allocator, LogType &log,
                         SMStateType &State) {
  AVLTStateKind CurrentState = State.getStateKind();
  switch (CurrentState) {
  case AVLTStateKind::C0:
    assert(false);
    break;
  case AVLTStateKind::A1:
    processStateA1(Allocator, log, State);
    break;
  case AVLTStateKind::A2:
    processStateA2(Allocator, log, State);
    break;
  case AVLTStateKind::A3:
    processStateA3(Allocator, log, State);
    break;
  case AVLTStateKind::A4:
    processStateA4(Allocator, log, State);
    break;
  case AVLTStateKind::A5:
    processStateA5(Allocator, log, State);
    break;
  case AVLTStateKind::A6:
    processStateA6(Allocator, log, State);
    break;
  case AVLTStateKind::A7:
    processStateA7(Allocator, log, State);
    break;
  case AVLTStateKind::A8:
    processStateA8(Allocator, log, State);
    break;
  case AVLTStateKind::A9:
    processStateA9(Allocator, log, State);
    break;
  case AVLTStateKind::A10:
    processStateA10(Allocator, log, State);
    break;
  case AVLTStateKind::A11:
    processStateA11(Allocator, log, State);
    break;
  case AVLTStateKind::A12:
    processStateA12(Allocator, log, State);
    break;
  case AVLTStateKind::A13:
    processStateA13(Allocator, log, State);
    break;
  case AVLTStateKind::A14:
    processStateA14(Allocator, log, State);
    break;
  case AVLTStateKind::A15:
    processStateA15(Allocator, log, State);
    break;
  case AVLTStateKind::A16:
    processStateA16(Allocator, log, State);
    break;
  case AVLTStateKind::A17:
    processStateA17(Allocator, log, State);
    break;
  case AVLTStateKind::A18:
    processStateA18(Allocator, log, State);
    break;
  case AVLTStateKind::A19:
    processStateA19(Allocator, log, State);
    break;
  case AVLTStateKind::A20:
    processStateA20(Allocator, log, State);
    break;
  case AVLTStateKind::A21:
    processStateA21(Allocator, log, State);
    break;
  case AVLTStateKind::A22:
    processStateA22(Allocator, log, State);
    break;
  case AVLTStateKind::A23:
    processStateA23(Allocator, log, State);
    break;
  case AVLTStateKind::A24:
    processStateA24(Allocator, log, State);
    break;
  case AVLTStateKind::A25:
    processStateA25(Allocator, log, State);
    break;
  case AVLTStateKind::A26:
    processStateA26(Allocator, log, State);
    break;
  case AVLTStateKind::R1:
    assert(false);
    break;
  case AVLTStateKind::R2:
    assert(false);
    break;
  case AVLTStateKind::R3:
    assert(false);
    break;
  case AVLTStateKind::R4:
    assert(false);
    break;
  case AVLTStateKind::R5:
    assert(false);
    break;
  case AVLTStateKind::R6:
    assert(false);
    break;
  case AVLTStateKind::R7:
    assert(false);
    break;
  case AVLTStateKind::R8:
    assert(false);
    break;
  case AVLTStateKind::R9:
    assert(false);
    break;
  case AVLTStateKind::R10:
    assert(false);
    break;
  case AVLTStateKind::R11:
    assert(false);
    break;
  case AVLTStateKind::R12:
    assert(false);
    break;
  case AVLTStateKind::R13:
    assert(false);
    break;
  case AVLTStateKind::R14:
    assert(false);
    break;
  case AVLTStateKind::R15:
    assert(false);
    break;
  case AVLTStateKind::R16:
    assert(false);
    break;
  case AVLTStateKind::R17:
    assert(false);
    break;
  case AVLTStateKind::R18:
    assert(false);
    break;
  case AVLTStateKind::R19:
    assert(false);
    break;
  case AVLTStateKind::R20:
    assert(false);
    break;
  case AVLTStateKind::R21:
    assert(false);
    break;
  case AVLTStateKind::R22:
    assert(false);
    break;
  case AVLTStateKind::R23:
    assert(false);
    break;
  case AVLTStateKind::R24:
    assert(false);
    break;
  case AVLTStateKind::R25:
    assert(false);
    break;
  case AVLTStateKind::R26:
    assert(false);
    break;
  case AVLTStateKind::R27:
    assert(false);
    break;
  case AVLTStateKind::R28:
    assert(false);
    break;
  case AVLTStateKind::R29:
    assert(false);
    break;
  case AVLTStateKind::R30:
    assert(false);
    break;
  case AVLTStateKind::R31:
    assert(false);
    break;
  case AVLTStateKind::R32:
    assert(false);
    break;
  case AVLTStateKind::R33:
    assert(false);
    break;
  case AVLTStateKind::R34:
    assert(false);
    break;
  }
}

} // namespace

namespace Avlt::Insert {

void GoToC0(AllocatorType &Allocator, LogType &log, SMStateType &State,
            uint64_t Key, uint64_t Value) {
  AVLTStateKind CurrentState = State.getStateKind();

  assert(CurrentState == AVLTStateKind::C0);

  processStateC0(log, State, Key, Value);
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

} // namespace Avlt::Insert
