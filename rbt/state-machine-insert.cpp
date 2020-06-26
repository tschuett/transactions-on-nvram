#include "state-machine-insert.h"

#include "flush.h"
#include "plot.h"
#include "rbt-allocator.h"
#include "rbt-log.h"
#include "recover.h"
#include "state-kind.h"
#include "tree.h"
#include "write-dsl-rbt.h"

#include <cstdint>

namespace {

bool canDoAnotherStep(AllocatorType &Allocator, Iterators Its, uint64_t Key) {
  if (IsNullPtr<NodePtrValue>(NodePtrValue(Its.QP)).readValue())
    // new node at the bottom
    return false;
  else if (IsRed<LeftInNode<NodePtrValue>>(
               Allocator,
               LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Its.QP)))
               .readValue() &&
           IsRed<RightInNode<NodePtrValue>>(
               Allocator,
               RightInNode<NodePtrValue>(Allocator, NodePtrValue(Its.QP)))
               .readValue())
    // Color flip
    return false;
  else if (IsRed<NodePtrValue>(Allocator, NodePtrValue(Its.QP)).readValue() &&
           IsRed<NodePtrValue>(Allocator, NodePtrValue(Its.ParentP))
               .readValue()) {
    Direction dir2 = Direction::Unknown;

    if (EqualNodes<RightInNode<NodePtrValue>, NodePtrValue>(
            RightInNode<NodePtrValue>(Allocator, NodePtrValue(Its.Tp)),
            NodePtrValue(Its.GrandP))
            .readValue())
      dir2 = Direction::Right;
    else
      dir2 = Direction::Left;

    if (EqualNodes<NodePtrValue, ChildInNode<NodePtrValue, DirectionValue>>(
            NodePtrValue(Its.QP), ChildInNode<NodePtrValue, DirectionValue>(
                                      Allocator, DirectionValue(Its.LastDir),
                                      NodePtrValue(Its.ParentP)))
            .readValue()) {
      // allocator.get(t)->right = singleRotation(allocator, g,
      // negate(last));
      return false;
    } else {
      // allocator.get(t)->right = doubleRotation(allocator, g,
      // negate(last));
      return false;
    }
  } else {
    if (EqualKey<KeyInNode<NodePtrValue>, KeyValue>(
            KeyInNode<NodePtrValue>(Allocator, NodePtrValue(Its.QP)),
            KeyValue(Key))
            .readValue())
      return false;
    else
      return true;
  }
  assert(false);
}

Iterators goDownOneLevel(AllocatorType &Allocator, Iterators Current,
                         uint64_t Key) {
  Direction Dir = Direction::Unknown;
  if (SmallerThanKey<KeyInNode<NodePtrValue>, KeyValue>(
          KeyInNode<NodePtrValue>(Allocator, NodePtrValue(Current.QP)),
          KeyValue(Key))
          .readValue())
    Dir = Direction::Right;
  else
    Dir = Direction::Left;

  NodeIndex t = Current.Tp;
  if (Current.GrandP != Nullptr)
    t = Current.GrandP;

  NodeIndex NextQP =
      ChildInNode<NodePtrValue, DirectionValue>(Allocator, DirectionValue(Dir),
                                                NodePtrValue(Current.QP))
          .readValue();

  Iterators NextIterators;
  NextIterators.LastDir = Current.Dir;
  NextIterators.Dir = Dir;
  NextIterators.Tp = t;
  NextIterators.GrandP = Current.ParentP;
  NextIterators.ParentP = Current.QP;
  NextIterators.QP = NextQP;
  NextIterators.Q2P = NextQP;

  assert(NextQP != Current.QP);

  return NextIterators;
}

void transitionC0ToA1(LogType &log, uint64_t key, uint64_t value) {
  // LogStructure *Log = log.getLog();
  // flush(
  //    [&] {
  //      write(&log_->Key, key);
  //      write(&log_->Value, value);
  //    },
  //    FlushKind::FlushCommand);

  WriteOp<LogAddress::Key, KeyValue> W1 = {LogAddress::Key(log), KeyValue(key)};
  WriteOp<LogAddress::Value, ValueValue> W2 = {LogAddress::Value(log),
                                               ValueValue(value)};

  flushOp(FlushKind::FlushCommand, W1, W2);
}

void transitionC0ToA2(LogType &log, uint64_t key, uint64_t value) {
  // LogStructure *Log = log.getLog();
  // flush(
  //     [&] {
  //       write(&Log->Key, key);
  //       write(&Log->Value, value);
  //     },
  //     FlushKind::FlushCommand);

  WriteOp<LogAddress::Key, KeyValue> W1 = {LogAddress::Key(log), KeyValue(key)};
  WriteOp<LogAddress::Value, ValueValue> W2 = {LogAddress::Value(log),
                                               ValueValue(value)};

  flushOp(FlushKind::FlushCommand, W1, W2);
}

void transitionA1ToC0(AllocatorType &Allocator, LogType &log) {
  // LogStructure *Log = log.getLog();
  NodeIndex r = Allocator.getNewNodeFromLog(); // memory leak. so what
  // flush(
  //    [&] {
  //      write(&Log->Root, r);
  //      write(&Allocator.get(r)->color, Color::Black);
  //    },
  //    FlushKind::InsertNewNode);

  WriteOp<LogAddress::Root, NodePtrValue> W1 = {LogAddress::Root(log),
                                                NodePtrValue(r)};
  WriteOp<ColorInNode<NodePtrValue>, ColorValue> W2 = {
      ColorInNode<NodePtrValue>(Allocator, NodePtrValue(r)),
      ColorValue(Color::Black)};

  flushOp(FlushKind::InsertNewNode, W1, W2);
}

void transitionA2ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // LogStructure *Log = log.getLog();
  NodeIndex r = Allocator.getHeadNode();
  uint32_t EvenOdd = State.getEvenOddBit();
  Iterators Its;
  Its.LastDir = Direction::Unknown;
  Its.Dir = Direction::Left;
  Its.Tp = r;
  Its.GrandP = Nullptr;
  Its.ParentP = Nullptr;
  Its.QP = LogAddress::Root(log).readValue();
  Its.Q2P = LogAddress::Root(log).readValue(); // billiger hack
  Its.Fp = Nullptr;

  //   flush(
  //       [&] {
  //         write(&Log->Iterator[EvenOdd].Dir, Direction::Left);
  //         write(&Log->Iterator[EvenOdd].Tp, r);
  //         write(&Log->Iterator[EvenOdd].GrandP, Nullptr);
  //         write(&Log->Iterator[EvenOdd].ParentP, Nullptr);
  //         write(&Log->Iterator[EvenOdd].QP, Log->Root);
  //         write(&Log->Iterator[EvenOdd].Q2P, Log->Root); // billiger hack
  //         write(&Allocator.get(r)->right, Log->Root);
  //       },
  //       FlushKind::InitializeVariables);

  WriteOp<LogAddress::Iterators, IteratorsValue> W1 = {
      LogAddress::Iterators(log, EvenOdd), IteratorsValue(Its)};
  WriteOp<RightInNode<NodePtrValue>, LogAddress::Root> W2 = {
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(r)),
      LogAddress::Root(log)};

  // printf("A2 -> X: Tp = %u\n", Its.Tp.getValue());

  flushOp(FlushKind::InitializeVariables, W1, W2);
}

void transitionA4ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // LogStructure *Log = log.getLog();
  NodeIndex n = Allocator.getNewNodeFromLog(); // memory leak. so what
  uint32_t EvenOdd = State.getEvenOddBit();

  if (HasDirection<LogAddress::Dir>(LogAddress::Dir(log, EvenOdd),
                                    Direction::Left)
          .readValue()) {
    // if (Log->Iterator[EvenOdd].Dir == Direction::Left) {
    // flush(
    //     [&] {
    //       write(&Allocator.get(Log->Iterator[EvenOdd].ParentP)->left, n);
    //       write(&Log->Iterator[EvenOdd].QP, n);
    //     },
    //     FlushKind::InsertNewNode);

    WriteOp<LeftInNode<LogAddress::ParentP>, NodePtrValue> W1 = {
        LeftInNode<LogAddress::ParentP>(Allocator,
                                        LogAddress::ParentP(log, EvenOdd)),
        NodePtrValue(n)};
    WriteOp<LogAddress::QP, NodePtrValue> W2 = {LogAddress::QP(log, EvenOdd),
                                                NodePtrValue(n)};

    flushOp(FlushKind::InsertNewNode, W1, W2);

  } else if (HasDirection<LogAddress::Dir>(LogAddress::Dir(log, EvenOdd),
                                           Direction::Right)
                 .readValue()) {
    // flush(
    //    [&] {
    //      write(&Allocator.get(Log->Iterator[EvenOdd].ParentP)->right, n);
    //      write(&Log->Iterator[EvenOdd].QP, n);
    //    },
    //    FlushKind::InsertNewNode);

    WriteOp<RightInNode<LogAddress::ParentP>, NodePtrValue> W1 = {
        RightInNode<LogAddress::ParentP>(Allocator,
                                         LogAddress::ParentP(log, EvenOdd)),
        NodePtrValue(n)};
    WriteOp<LogAddress::QP, NodePtrValue> W2 = {LogAddress::QP(log, EvenOdd),
                                                NodePtrValue(n)};

    flushOp(FlushKind::InsertNewNode, W1, W2);

  } else
    assert(false);
}

/* Color flip */
void transitionA5ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // LogStructure *Log = log.getLog();
  uint32_t EvenOdd = State.getEvenOddBit();

  // assert(Log->Iterator[EvenOdd].Q2P != Nullptr);
  // flush(
  //     [&] {
  //       write(&Allocator.get(Log->Iterator[EvenOdd].Q2P)->color,
  //       Color::Red);
  //       write(&Allocator.get(Allocator.get(Log->Iterator[EvenOdd].Q2P)->left)
  //                  ->color,
  //             Color::Black);
  //       write(&Allocator.get(Allocator.get(Log->Iterator[EvenOdd].Q2P)->right)
  //                  ->color,
  //             Color::Black);
  //     },
  //     FlushKind::ColorFlip);

  WriteOp<ColorInNode<LogAddress::Q2P>, ColorValue> W1 = {
      ColorInNode<LogAddress::Q2P>(Allocator, LogAddress::Q2P(log, EvenOdd)),
      ColorValue(Color::Red)};
  WriteOp<ColorInNode<LeftInNode<LogAddress::Q2P>>, ColorValue> W2 = {
      ColorInNode<LeftInNode<LogAddress::Q2P>>(
          Allocator, LeftInNode<LogAddress::Q2P>(
                         Allocator, LogAddress::Q2P(log, EvenOdd))),
      ColorValue(Color::Black)};
  WriteOp<ColorInNode<RightInNode<LogAddress::Q2P>>, ColorValue> W3 = {
      ColorInNode<RightInNode<LogAddress::Q2P>>(
          Allocator, RightInNode<LogAddress::Q2P>(
                         Allocator, LogAddress::Q2P(log, EvenOdd))),
      ColorValue(Color::Black)};

  flushOp(FlushKind::ColorFlip, W1, W2, W3);
}

/* flush for single rotation */
void transitionA10ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();
  Direction dir2 = Direction::Unknown;
  Direction Dir =
      NegateDirection<LogAddress::LastDir>(LogAddress::LastDir(log, EvenOdd))
          .readValue();
  NodeIndex tmp = Nullptr;
  NodeIndex save = Nullptr;

  if (EqualNodes<RightInNode<LogAddress::Tp>, LogAddress::GrandP>(
          RightInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          LogAddress::GrandP(log, EvenOdd))
          .readValue())
    dir2 = Direction::Right;
  else
    dir2 = Direction::Left;

  if (Dir == Direction::Left)
    save = RightInNode<LogAddress::GrandP>(Allocator,
                                           LogAddress::GrandP(log, EvenOdd))
               .readValue();
  else if (Dir == Direction::Right)
    save = LeftInNode<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd))
               .readValue();
  else
    assert(false);

  if (Dir == Direction::Left)
    tmp = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(save)).readValue();
  else if (Dir == Direction::Right)
    tmp = RightInNode<NodePtrValue>(Allocator, NodePtrValue(save)).readValue();
  else
    assert(false);

  // flush(
  //     [&] {
  //       write(&Log->Log.SaveP, save);
  //       write(&Log->Log.Dir2, dir2);
  //       write(&Log->Log.SaveChildP, tmp);
  //       //write(&Log->Log.Dir2, dir2);
  //     },
  //     FlushKind::SingleRotation);

  WriteOp<LogAddress::SaveP, NodePtrValue> W1 = {LogAddress::SaveP(log),
                                                 NodePtrValue(save)};

  WriteOp<LogAddress::Dir2, DirectionValue> W2 = {LogAddress::Dir2(log),
                                                  DirectionValue(dir2)};

  WriteOp<LogAddress::SaveChildP, NodePtrValue> W3 = {
      LogAddress::SaveChildP(log), NodePtrValue(tmp)};

  flushOp(FlushKind::SingleRotation, W1, W2, W3);
}

/* first flush for double rotation */
void transitionA6ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // LogStructure *Log = log.getLog();
  uint32_t EvenOdd = State.getEvenOddBit();
  Direction dir2 = Direction::Unknown;
  Direction Dir =
      NegateDirection<LogAddress::LastDir>(LogAddress::LastDir(log, EvenOdd))
          .readValue();
  NodeIndex root = Nullptr;
  NodeIndex Save = Nullptr;
  NodeIndex SaveChild = Nullptr;

  if (EqualNodes<RightInNode<LogAddress::Tp>, LogAddress::GrandP>(
          RightInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          LogAddress::GrandP(log, EvenOdd))
          .readValue())
    dir2 = Direction::Right;
  else
    dir2 = Direction::Left;

  assert(LogAddress::GrandP(log, EvenOdd).readValue() != Nullptr);

  if (Dir == Direction::Left)
    root = RightInNode<LogAddress::GrandP>(Allocator,
                                           LogAddress::GrandP(log, EvenOdd))
               .readValue();
  else if (Dir == Direction::Right)
    root = LeftInNode<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd))
               .readValue();
  else
    assert(false);

  assert(root != Nullptr);

  if (Dir == Direction::Left)
    Save = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(root)).readValue();
  else if (Dir == Direction::Right)
    Save = RightInNode<NodePtrValue>(Allocator, NodePtrValue(root)).readValue();
  else
    assert(false);

  assert(Save != Nullptr);

  if (Dir == Direction::Left)
    SaveChild =
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
  else if (Dir == Direction::Right)
    SaveChild =
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();

  // flush(
  //    [&] {
  //      write(&Log->Log.TmpNode,
  //      *Allocator.get(Log->Iterator[EvenOdd].GrandP));
  //      write(&Log->Log.Dir2, dir2); write(&Log->Log.SaveP, Save);
  //      write(&Log->Log.SaveChildP, SaveChild);
  //    },
  //    FlushKind::DoubleRotation);

  WriteOp<LogAddress::TmpNode, NodeFromAddress<LogAddress::GrandP>> W1 = {
      LogAddress::TmpNode(log),
      NodeFromAddress<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd))};

  WriteOp<LogAddress::Dir2, DirectionValue> W2 = {LogAddress::Dir2(log),
                                                  DirectionValue(dir2)};

  WriteOp<LogAddress::SaveP, NodePtrValue> W3 = {LogAddress::SaveP(log),
                                                 NodePtrValue(Save)};

  WriteOp<LogAddress::SaveChildP, NodePtrValue> W4 = {
      LogAddress::SaveChildP(log), NodePtrValue(SaveChild)};

  flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4);
}

// flush q to the log as q2p
void transitionA3ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  // LogStructure *Log = log.getLog();
  uint32_t EvenOdd = State.getEvenOddBit();
  // flush([&] { write(&Log->Iterator[EvenOdd].Q2P,
  // Log->Iterator[EvenOdd].QP);
  // },
  //      FlushKind::Misc);

  WriteOp<LogAddress::Q2P, LogAddress::QP> W1 = {LogAddress::Q2P(log, EvenOdd),
                                                 LogAddress::QP(log, EvenOdd)};

  flushOp(FlushKind::Misc, W1);
}

// single rotation: rotate
void transitionA11ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  // LogStructure *Log = log.getLog();
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction Dir =
      NegateDirection<LogAddress::LastDir>(LogAddress::LastDir(log, EvenOdd))
          .readValue();
  Direction Dir2 = LogAddress::Dir2(log).readValue();

  NodeIndex Root = LogAddress::GrandP(log, EvenOdd).readValue();
  NodeIndex Save = LogAddress::SaveP(log).readValue();
  NodeIndex SaveChild = LogAddress::SaveChildP(log).readValue();

  assert(Save != Nullptr);

  if (Dir2 == Direction::Right) {
    if (Dir == Direction::Left) {
      // flush(
      //    [&] {
      //      write(&Allocator.get(Root)->right, SaveChild);
      //      write(&Allocator.get(Save)->left, Root);
      //      write(&Allocator.get(Root)->color, Color::Red);
      //      write(&Allocator.get(Save)->color, Color::Black);
      //      write(&Allocator.get(Log->Iterator[EvenOdd].Tp)->right, Save);
      //    },
      //    FlushKind::SingleRotation);

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          NodePtrValue(SaveChild)};

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(Root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<RightInNode<LogAddress::Tp>, NodePtrValue> W5 = {
          RightInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5);

    } else if (Dir == Direction::Right) {
      // flush(
      //     [&] {
      //       write(&Allocator.get(Root)->left, SaveChild);
      //       write(&Allocator.get(Save)->right, Root);
      //       write(&Allocator.get(Root)->color, Color::Red);
      //       write(&Allocator.get(Save)->color, Color::Black);
      //       write(&Allocator.get(Log->Iterator[EvenOdd].Tp)->right, Save);
      //     },
      //     FlushKind::SingleRotation);

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          NodePtrValue(SaveChild)};

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(Root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<RightInNode<LogAddress::Tp>, NodePtrValue> W5 = {
          RightInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5);
    }
  } else if (Dir2 == Direction::Left) {
    if (Dir == Direction::Left) {
      // flush(
      //     [&] {
      //       write(&Allocator.get(Root)->right, SaveChild);
      //       write(&Allocator.get(Save)->left, Root);
      //       write(&Allocator.get(Root)->color, Color::Red);
      //       write(&Allocator.get(Save)->color, Color::Black);
      //       write(&Allocator.get(Log->Iterator[EvenOdd].Tp)->left, Save);
      //     },
      //     FlushKind::SingleRotation);

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          NodePtrValue(SaveChild)};

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(Root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<LeftInNode<LogAddress::Tp>, NodePtrValue> W5 = {
          LeftInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5);

    } else if (Dir == Direction::Right) {
      // flush(
      //     [&] {
      //       write(&Allocator.get(Root)->left, SaveChild);
      //       write(&Allocator.get(Save)->right, Root);
      //       write(&Allocator.get(Root)->color, Color::Red);
      //       write(&Allocator.get(Save)->color, Color::Black);
      //       write(&Allocator.get(Log->Iterator[EvenOdd].Tp)->left, Save);
      //     },
      //     FlushKind::SingleRotation);

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          NodePtrValue(SaveChild)};

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(Root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<LeftInNode<LogAddress::Tp>, NodePtrValue> W5 = {
          LeftInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5);
    }
  }
}

// double rotation: first rotate
void transitionA7ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction Dir =
      NegateDirection<LogAddress::LastDir>(LogAddress::LastDir(log, EvenOdd))
          .readValue();

  NodeIndex Root = Nullptr;
  NodeIndex Save = LogAddress::SaveP(log).readValue();
  NodeIndex SaveChild = LogAddress::SaveChildP(log).readValue();

  if (Dir == Direction::Left)
    Root = RightInNode<LogAddress::TmpNode>(log).readValue();
  else if (Dir == Direction::Right)
    Root = LeftInNode<LogAddress::TmpNode>(log).readValue();
  else
    assert(false);

  assert(Root != Nullptr);
  assert(Save != Nullptr);

  if (Dir == Direction::Left) {
    // flush(
    //     [&] {
    //       write(&Allocator.get(Root)->right, SaveChild);
    //       write(&Allocator.get(Save)->left, Root);
    //       write(&Allocator.get(Root)->color, Color::Red);
    //       write(&Allocator.get(Save)->color, Color::Black);
    //       write(&Allocator.get(Log->Iterator[EvenOdd].ParentP)->left,
    //       Save);
    //     },
    //     FlushKind::DoubleRotation);

    WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
        NodePtrValue(SaveChild)};

    WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        NodePtrValue(Root)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
        ColorValue(Color::Red)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        ColorValue(Color::Black)};

    WriteOp<RightInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
        RightInNode<LogAddress::GrandP>(Allocator,
                                        LogAddress::GrandP(log, EvenOdd)),
        NodePtrValue(Save)};

    flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);

  } else if (Dir == Direction::Right) {

    WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
        NodePtrValue(SaveChild)};

    WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        NodePtrValue(Root)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
        ColorValue(Color::Red)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        ColorValue(Color::Black)};

    WriteOp<LeftInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
        LeftInNode<LogAddress::GrandP>(Allocator,
                                       LogAddress::GrandP(log, EvenOdd)),
        NodePtrValue(Save)};

    flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);

    //
    //           flush(
    //         [&] {
    //           write(&Allocator.get(Root)->left, SaveChild);
    //           write(&Allocator.get(Save)->right, Root);
    //           write(&Allocator.get(Root)->color, Color::Red);
    //           write(&Allocator.get(Save)->color, Color::Black);
    //           write(&Allocator.get(Log->Iterator[EvenOdd].ParentP)->right,
    //           Save);
    //         },
    //         FlushKind::DoubleRotation);
  }
}

// double rotation: second rotate
void transitionA9ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction Dir =
      NegateDirection<LogAddress::LastDir>(LogAddress::LastDir(log, EvenOdd))
          .readValue();
  Direction Dir2 = LogAddress::Dir2(log).readValue();

  NodeIndex Save = Nullptr;
  NodeIndex Root = LogAddress::GrandP(log, EvenOdd).readValue();
  NodeIndex SaveChild = LogAddress::SaveChildP(log).readValue();
  if (Dir == Direction::Left)
    Save = RightInNode<LogAddress::TmpNode>(log).readValue();
  else if (Dir == Direction::Right)
    Save = LeftInNode<LogAddress::TmpNode>(log).readValue();
  else
    assert(false);

  assert(Root != Nullptr);
  assert(Save != Nullptr);

  if (Dir2 == Direction::Right) {
    if (Dir == Direction::Left) {
      // flush(
      //     [&] {
      //       write(&Allocator.get(Root)->right, SaveChild);
      //       write(&Allocator.get(Save)->left, Root);
      //       write(&Allocator.get(Root)->color, Color::Red);
      //       write(&Allocator.get(Save)->color, Color::Black);
      //       write(&Allocator.get(Log->Iterator[EvenOdd].Tp)->right, Save);
      //     },
      //     FlushKind::DoubleRotation);

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          NodePtrValue(SaveChild)};

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(Root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<RightInNode<LogAddress::Tp>, NodePtrValue> W5 = {
          RightInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);

    } else if (Dir == Direction::Right) {
      // flush(
      //     [&] {
      //       write(&Allocator.get(Root)->left, SaveChild);
      //       write(&Allocator.get(Save)->right, Root);
      //       write(&Allocator.get(Root)->color, Color::Red);
      //       write(&Allocator.get(Save)->color, Color::Black);
      //       write(&Allocator.get(Log->Iterator[EvenOdd].Tp)->right, Save);
      //     },
      //     FlushKind::DoubleRotation);

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          NodePtrValue(SaveChild)};

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(Root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<RightInNode<LogAddress::Tp>, NodePtrValue> W5 = {
          RightInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);
    }
  } else if (Dir2 == Direction::Left) {
    if (Dir == Direction::Left) {
      // flush(
      //     [&] {
      //       write(&Allocator.get(Root)->right, SaveChild);
      //       write(&Allocator.get(Save)->left, Root);
      //       write(&Allocator.get(Root)->color, Color::Red);
      //       write(&Allocator.get(Save)->color, Color::Black);
      //       write(&Allocator.get(Log->Iterator[EvenOdd].Tp)->left, Save);
      //     },
      //     FlushKind::DoubleRotation);

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          NodePtrValue(SaveChild)};

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(Root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<LeftInNode<LogAddress::Tp>, NodePtrValue> W5 = {
          LeftInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);

    } else if (Dir == Direction::Right) {
      // flush(
      //     [&] {
      //       write(&Allocator.get(Root)->left, SaveChild);
      //       write(&Allocator.get(Save)->right, Root);
      //       write(&Allocator.get(Root)->color, Color::Red);
      //       write(&Allocator.get(Save)->color, Color::Black);
      //       write(&Allocator.get(Log->Iterator[EvenOdd].Tp)->left, Save);
      //     },
      //     FlushKind::DoubleRotation);

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          NodePtrValue(SaveChild)};

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(Root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<LeftInNode<LogAddress::Tp>, NodePtrValue> W5 = {
          LeftInNode<LogAddress::Tp>(Allocator, LogAddress::Tp(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);
    }
  }
}

// flush to disk after if statements
void transitionA12ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();
  uint32_t NextEvenOdd = State.getFlippedEvenOddBit();
  Iterators Its;

  Iterators Current = LogAddress::Iterators(log, EvenOdd).readValue();
  uint64_t Key = LogAddress::Key(log).readValue();
  if (canDoAnotherStep(Allocator, Current, Key)) {
    while (canDoAnotherStep(Allocator, Current, Key)) {
      Current = goDownOneLevel(Allocator, Current, Key);
    }

    Its.LastDir = Current.LastDir;
    Its.Dir = Current.Dir;
    Its.Tp = Current.Tp;
    Its.GrandP = Current.GrandP;
    Its.ParentP = Current.ParentP;
    Its.QP = Current.QP;
    Its.Q2P = Current.Q2P;
  } else {
    Its = goDownOneLevel(Allocator, Current, Key);
  }

  WriteOp<LogAddress::Iterators, IteratorsValue> W1 = {
      LogAddress::Iterators(log, NextEvenOdd), IteratorsValue(Its)};

  flushOp(FlushKind::ShuffleIterators, W1);
}

// after for loop
void transitionA13ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  /* Update root */
  // LogStructure *Log = log.getLog();
  NodeIndex r = Allocator.getHeadNode();

  // flush([&] { write(&Log->Root, Allocator.get(r)->right); },
  //       FlushKind::UpdateRoot);

  WriteOp<LogAddress::Root, RightInNode<NodePtrValue>> W1 = {
      LogAddress::Root(log),
      RightInNode<NodePtrValue>(Allocator, NodePtrValue(r))};

  flushOp(FlushKind::UpdateRoot, W1);
}

// after the major if statement
void transitionA14ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  /* Make root black */
  // LogStructure *Log = log.getLog();

  // flush([&] { write(&Allocator.get(Log->Root)->color, Color::Black); },
  //       FlushKind::UpdateRoot);

  WriteOp<ColorInNode<LogAddress::Root>, ColorValue> W1 = {
      ColorInNode<LogAddress::Root>(Allocator, LogAddress::Root(log)),
      ColorValue(Color::Black)};

  flushOp(FlushKind::UpdateRoot, W1);
}

/* second flush for second double rotation */
void transitionA8ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  NodeIndex Root = Nullptr;
  NodeIndex Save = Nullptr;
  NodeIndex SaveChild = Nullptr;
  Direction Dir = LogAddress::LastDir(log, EvenOdd).readValue();

  Root = LogAddress::GrandP(log, EvenOdd).readValue();

  assert(Root != Nullptr);

  if (Dir == Direction::Left)
    Save = LeftInNode<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd))
               .readValue();
  else if (Dir == Direction::Right)
    Save = RightInNode<LogAddress::GrandP>(Allocator,
                                           LogAddress::GrandP(log, EvenOdd))
               .readValue();
  else
    assert(false);

  assert(Save != Nullptr);

  if (Dir == Direction::Left)
    SaveChild =
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
  else if (Dir == Direction::Right)
    SaveChild =
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();

  // flush(
  //     [&] {
  //       write(&Log->Log.TmpNode,
  //       *Allocator.get(Log->Iterator[EvenOdd].GrandP));
  //       write(&Log->Log.SaveChildP, SaveChild);
  //     },
  //     FlushKind::DoubleRotation);

  WriteOp<LogAddress::TmpNode, NodeFromAddress<LogAddress::GrandP>> W1 = {
      LogAddress::TmpNode(log),
      NodeFromAddress<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd))};

  WriteOp<LogAddress::SaveChildP, NodePtrValue> W2 = {
      LogAddress::SaveChildP(log), NodePtrValue(SaveChild)};

  flushOp(FlushKind::UpdateRoot, W1, W2);
}

void processStateC0(LogType &log, SMStateType &State, uint64_t Key,
                    uint64_t Value) {
  // LogStructure *Log = log.getLog();

  // two successors depending on empty tree or not
  if (IsNullPtr<LogAddress::Root>(LogAddress::Root(log)).readValue()) {
    // flush k+v to disk
    transitionC0ToA1(log, Key, Value);
    State.changeWoEvenOdd(RBTStateKind::C0, RBTStateKind::A1);
  } else {
    // flush k+v to disk
    transitionC0ToA2(log, Key, Value);
    State.changeWoEvenOdd(RBTStateKind::C0, RBTStateKind::A2);
  }
}

void processStateA1(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // insert new root node
  transitionA1ToC0(Allocator, log);
  State.changeWoEvenOdd(RBTStateKind::A1, RBTStateKind::C0);
}

void processStateA2(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  // initialize all variables
  transitionA2ToDX(Allocator, log, State);

  NodeIndex QP = LogAddress::QP(log, EvenOdd).readValue();

  if (IsNullPtr<NodePtrValue>(NodePtrValue(QP)).readValue())
    // new node at the bottom
    State.changeWoEvenOdd(RBTStateKind::A2, RBTStateKind::A3);
  else if (IsRed<LeftInNode<NodePtrValue>>(
               Allocator, LeftInNode<NodePtrValue>(Allocator, NodePtrValue(QP)))
               .readValue() &&
           IsRed<RightInNode<NodePtrValue>>(
               Allocator,
               RightInNode<NodePtrValue>(Allocator, NodePtrValue(QP)))
               .readValue())
    State.changeWoEvenOdd(RBTStateKind::A2, RBTStateKind::A5);
  else if (IsRed<NodePtrValue>(Allocator, NodePtrValue(QP)).readValue() &&
           IsRed<LogAddress::ParentP>(Allocator,
                                      LogAddress::ParentP(log, EvenOdd))
               .readValue()) {
    Direction dir2 = Direction::Unknown;

    if (EqualNodes<RightInNode<LogAddress::Tp>, LogAddress::GrandP>(
            RightInNode<LogAddress::Tp>(Allocator,
                                        LogAddress::Tp(log, EvenOdd)),
            LogAddress::GrandP(log, EvenOdd))
            .readValue())
      dir2 = Direction::Right;
    else
      dir2 = Direction::Left;

    if (EqualNodes<NodePtrValue,
                   ChildInNode<LogAddress::ParentP, LogAddress::LastDir>>(
            NodePtrValue(QP),
            ChildInNode<LogAddress::ParentP, LogAddress::LastDir>(
                Allocator, LogAddress::LastDir(log, EvenOdd),
                LogAddress::ParentP(log, EvenOdd)))
            .readValue()) {
      // allocator.get(t)->right = singleRotation(allocator, g,
      // negate(last));
      State.changeWoEvenOdd(RBTStateKind::A2, RBTStateKind::A10);
    } else {
      // allocator.get(t)->right = doubleRotation(allocator, g,
      // negate(last));
      State.changeWoEvenOdd(RBTStateKind::A2, RBTStateKind::A6);
    }
  } else {
    if (EqualKey<KeyInNode<NodePtrValue>, LogAddress::Key>(
            KeyInNode<NodePtrValue>(Allocator, NodePtrValue(QP)),
            LogAddress::Key(log))
            .readValue())
      State.changeWoEvenOdd(RBTStateKind::A2, RBTStateKind::A13);
    else
      State.changeWoEvenOdd(RBTStateKind::A2, RBTStateKind::A12);
  }
} // namespace

// q == Nullptr
void processStateA4(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionA4ToDX(Allocator, log, State);

  NodeIndex QP = LogAddress::QP(log, EvenOdd).readValue();
  if (IsRed<NodePtrValue>(Allocator, NodePtrValue(QP)).readValue() &&
      IsRed<LogAddress::ParentP>(Allocator, LogAddress::ParentP(log, EvenOdd))
          .readValue()) {
    Direction dir2 = Direction::Unknown;

    if (EqualNodes<RightInNode<LogAddress::Tp>, LogAddress::GrandP>(
            RightInNode<LogAddress::Tp>(Allocator,
                                        LogAddress::Tp(log, EvenOdd)),
            LogAddress::GrandP(log, EvenOdd))
            .readValue())
      dir2 = Direction::Right;
    else
      dir2 = Direction::Left;

    if (EqualNodes<NodePtrValue,
                   ChildInNode<LogAddress::ParentP, LogAddress::LastDir>>(
            NodePtrValue(QP),
            ChildInNode<LogAddress::ParentP, LogAddress::LastDir>(
                Allocator, LogAddress::LastDir(log, EvenOdd),
                LogAddress::ParentP(log, EvenOdd)))
            .readValue()) {
      // allocator.get(t)->right = singleRotation(allocator, g,
      // negate(last));
      State.changeWoEvenOdd(RBTStateKind::A4, RBTStateKind::A10);
    } else {
      // allocator.get(t)->right = doubleRotation(allocator, g,
      // negate(last));
      State.changeWoEvenOdd(RBTStateKind::A4, RBTStateKind::A6);
    }
  } else {
    if (EqualKey<KeyInNode<NodePtrValue>, LogAddress::Key>(
            KeyInNode<NodePtrValue>(Allocator, NodePtrValue(QP)),
            LogAddress::Key(log))
            .readValue())
      State.changeWoEvenOdd(RBTStateKind::A4, RBTStateKind::A13);
    else
      State.changeWoEvenOdd(RBTStateKind::A4, RBTStateKind::A12);
  }
}

// isRed(q->left) &&  isRed(q->right)
void processStateA5(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  // LogStructure *Log = log.getLog();
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionA5ToDX(Allocator, log, State);

  NodeIndex QP = LogAddress::QP(log, EvenOdd).readValue();
  if (IsRed<NodePtrValue>(Allocator, NodePtrValue(QP)).readValue() &&
      IsRed<LogAddress::ParentP>(Allocator, LogAddress::ParentP(log, EvenOdd))
          .readValue()) {
    Direction dir2 = Direction::Unknown;

    if (EqualNodes<RightInNode<LogAddress::Tp>, LogAddress::GrandP>(
            RightInNode<LogAddress::Tp>(Allocator,
                                        LogAddress::Tp(log, EvenOdd)),
            LogAddress::GrandP(log, EvenOdd))
            .readValue())
      dir2 = Direction::Right;
    else
      dir2 = Direction::Left;

    if (EqualNodes<NodePtrValue,
                   ChildInNode<LogAddress::ParentP, LogAddress::LastDir>>(
            NodePtrValue(QP),
            ChildInNode<LogAddress::ParentP, LogAddress::LastDir>(
                Allocator, LogAddress::LastDir(log, EvenOdd),
                LogAddress::ParentP(log, EvenOdd)))
            .readValue()) {
      // allocator.get(t)->right = singleRotation(allocator, g,
      // negate(last));
      State.changeWoEvenOdd(RBTStateKind::A5, RBTStateKind::A10);
    } else {
      // allocator.get(t)->right = doubleRotation(allocator, g,
      // negate(last));
      State.changeWoEvenOdd(RBTStateKind::A5, RBTStateKind::A6);
    }
  } else {
    if (EqualKey<KeyInNode<NodePtrValue>, LogAddress::Key>(
            KeyInNode<NodePtrValue>(Allocator, NodePtrValue(QP)),
            LogAddress::Key(log))
            .readValue())
      State.changeWoEvenOdd(RBTStateKind::A5, RBTStateKind::A13);
    else
      State.changeWoEvenOdd(RBTStateKind::A5, RBTStateKind::A12);
  }
}

// single rotation: flush to disk
void processStateA10(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA10ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::A10, RBTStateKind::A11);
}

// double rotation: flush to disk
void processStateA6(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA6ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::A6, RBTStateKind::A7);
}

// flush q to the log as q2p before the first ifs
void processStateA3(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionA3ToDX(Allocator, log, State);

  NodeIndex Q2P = LogAddress::Q2P(log, EvenOdd).readValue();
  if (IsNullPtr<NodePtrValue>(NodePtrValue(Q2P)).readValue())
    // new node at the bottom
    State.changeWoEvenOdd(RBTStateKind::A3, RBTStateKind::A4);
  else if (IsRed<LeftInNode<NodePtrValue>>(
               Allocator,
               LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Q2P)))
               .readValue() &&
           IsRed<RightInNode<NodePtrValue>>(
               Allocator,
               RightInNode<NodePtrValue>(Allocator, NodePtrValue(Q2P)))
               .readValue())
    // Color flip
    State.changeWoEvenOdd(RBTStateKind::A3, RBTStateKind::A5);
  else if (IsRed<NodePtrValue>(Allocator, NodePtrValue(Q2P)).readValue() &&
           IsRed<LogAddress::ParentP>(Allocator,
                                      LogAddress::ParentP(log, EvenOdd))
               .readValue()) {
    Direction dir2 = Direction::Unknown;

    if (EqualNodes<RightInNode<LogAddress::Tp>, LogAddress::GrandP>(
            RightInNode<LogAddress::Tp>(Allocator,
                                        LogAddress::Tp(log, EvenOdd)),
            LogAddress::GrandP(log, EvenOdd))
            .readValue())
      dir2 = Direction::Right;
    else
      dir2 = Direction::Left;

    if (EqualNodes<NodePtrValue,
                   ChildInNode<LogAddress::ParentP, LogAddress::LastDir>>(
            NodePtrValue(Q2P),
            ChildInNode<LogAddress::ParentP, LogAddress::LastDir>(
                Allocator, LogAddress::LastDir(log, EvenOdd),
                LogAddress::ParentP(log, EvenOdd)))
            .readValue()) {
      // if (Log->Iterator[EvenOdd].Q2P ==
      //    Allocator.get(Log->Iterator[EvenOdd].ParentP)
      //        ->getChild(Log->Iterator[EvenOdd].LastDir)) {
      // allocator.get(t)->right = singleRotation(allocator, g,
      // negate(last));
      State.changeWoEvenOdd(RBTStateKind::A3, RBTStateKind::A10);
    } else {
      // allocator.get(t)->right = doubleRotation(allocator, g,
      // negate(last));
      State.changeWoEvenOdd(RBTStateKind::A3, RBTStateKind::A6);
    }
  } else {
    if (EqualKey<KeyInNode<NodePtrValue>, LogAddress::Key>(
            KeyInNode<NodePtrValue>(Allocator, NodePtrValue(Q2P)),
            LogAddress::Key(log))
            .readValue())
      State.changeWoEvenOdd(RBTStateKind::A3, RBTStateKind::A13);
    else
      State.changeWoEvenOdd(RBTStateKind::A3, RBTStateKind::A12);
  }
} // namespace

// single rotation: rotate
void processStateA11(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionA11ToDX(Allocator, log, State);
  if (EqualKey<KeyInNode<LogAddress::QP>, LogAddress::Key>(
          KeyInNode<LogAddress ::QP>(Allocator, LogAddress::QP(log, EvenOdd)),
          LogAddress::Key(log))
          .readValue())
    State.changeWoEvenOdd(RBTStateKind::A11, RBTStateKind::A13);
  else
    State.changeWoEvenOdd(RBTStateKind::A11, RBTStateKind::A12);
}

// double rotation: first rotate
void processStateA7(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA7ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::A7, RBTStateKind::A8);
}

// double rotation: second rotate
void processStateA9(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionA9ToDX(Allocator, log, State);
  if (EqualKey<KeyInNode<LogAddress::QP>, LogAddress::Key>(
          KeyInNode<LogAddress ::QP>(Allocator, LogAddress::QP(log, EvenOdd)),
          LogAddress::Key(log))
          .readValue())
    State.changeWoEvenOdd(RBTStateKind::A9, RBTStateKind::A13);
  else
    State.changeWoEvenOdd(RBTStateKind::A9, RBTStateKind::A12);
}

// flush to disk after if statements
void processStateA12(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  uint32_t NextEvenOdd = State.getFlippedEvenOddBit();

  transitionA12ToDX(Allocator, log, State);

  NodeIndex QP = LogAddress::QP(log, NextEvenOdd).readValue();

  if (IsNullPtr<NodePtrValue>(NodePtrValue(QP)).readValue())
    // new node at the bottom
    State.changeWithEvenOdd(RBTStateKind::A12, RBTStateKind::A3);
  else if (IsRed<LeftInNode<NodePtrValue>>(
               Allocator, LeftInNode<NodePtrValue>(Allocator, NodePtrValue(QP)))
               .readValue() &&
           IsRed<RightInNode<NodePtrValue>>(
               Allocator,
               RightInNode<NodePtrValue>(Allocator, NodePtrValue(QP)))
               .readValue())
    // Color flip
    State.changeWithEvenOdd(RBTStateKind::A12, RBTStateKind::A5);
  else if (IsRed<NodePtrValue>(Allocator, NodePtrValue(QP)).readValue() &&
           IsRed<LogAddress::ParentP>(Allocator,
                                      LogAddress::ParentP(log, NextEvenOdd))
               .readValue()) {
    Direction dir2 = Direction::Unknown;

    if (EqualNodes<RightInNode<LogAddress::Tp>, LogAddress::GrandP>(
            RightInNode<LogAddress::Tp>(Allocator,
                                        LogAddress::Tp(log, NextEvenOdd)),
            LogAddress::GrandP(log, NextEvenOdd))
            .readValue())
      dir2 = Direction::Right;
    else
      dir2 = Direction::Left;

    if (EqualNodes<NodePtrValue,
                   ChildInNode<LogAddress::ParentP, LogAddress::LastDir>>(
            NodePtrValue(QP),
            ChildInNode<LogAddress::ParentP, LogAddress::LastDir>(
                Allocator, LogAddress::LastDir(log, NextEvenOdd),
                LogAddress::ParentP(log, NextEvenOdd)))
            .readValue()) {
      // allocator.get(t)->right = singleRotation(allocator, g,
      // negate(last));
      State.changeWithEvenOdd(RBTStateKind::A12, RBTStateKind::A10);
    } else {
      // allocator.get(t)->right = doubleRotation(allocator, g,
      // negate(last));
      State.changeWithEvenOdd(RBTStateKind::A12, RBTStateKind::A6);
    }
  } else {
    if (EqualKey<KeyInNode<NodePtrValue>, LogAddress::Key>(
            KeyInNode<NodePtrValue>(Allocator, NodePtrValue(QP)),
            LogAddress::Key(log))
            .readValue())
      State.changeWithEvenOdd(RBTStateKind::A12, RBTStateKind::A13);
    else
      State.changeWithEvenOddSame(RBTStateKind::A12, RBTStateKind::A12);
  }
}

// after for loop
void processStateA13(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA13ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::A13, RBTStateKind::A14);
}

// after the major if statement
void processStateA14(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionA14ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::A14, RBTStateKind::C0);
}

// double rotation: second flush to disk
void processStateA8(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionA8ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::A8, RBTStateKind::A9);
}

void processCurrentState(AllocatorType &Allocator, LogType &log,
                         SMStateType &State) {
  RBTStateKind CurrentState = State.getStateKind();
  switch (CurrentState) {
  case RBTStateKind::C0:
    assert(false);
    break;
  case RBTStateKind::A1:
    processStateA1(Allocator, log, State);
    break;
  case RBTStateKind::A2:
    processStateA2(Allocator, log, State);
    break;
  case RBTStateKind::A3:
    processStateA3(Allocator, log, State);
    break;
  case RBTStateKind::A4:
    processStateA4(Allocator, log, State);
    break;
  case RBTStateKind::A5:
    processStateA5(Allocator, log, State);
    break;
  case RBTStateKind::A6:
    processStateA6(Allocator, log, State);
    break;
  case RBTStateKind::A7:
    processStateA7(Allocator, log, State);
    break;
  case RBTStateKind::A8:
    processStateA8(Allocator, log, State);
    break;
  case RBTStateKind::A9:
    processStateA9(Allocator, log, State);
    break;
  case RBTStateKind::A10:
    processStateA10(Allocator, log, State);
    break;
  case RBTStateKind::A11:
    processStateA11(Allocator, log, State);
    break;
  case RBTStateKind::A12:
    processStateA12(Allocator, log, State);
    break;
  case RBTStateKind::A13:
    processStateA13(Allocator, log, State);
    break;
  case RBTStateKind::A14:
    processStateA14(Allocator, log, State);
    break;
  case RBTStateKind::R1:
  case RBTStateKind::R2:
  case RBTStateKind::R3:
  case RBTStateKind::R4:
  case RBTStateKind::R5:
  case RBTStateKind::R6:
  case RBTStateKind::R7:
  case RBTStateKind::R8:
  case RBTStateKind::R9:
  case RBTStateKind::R10:
  case RBTStateKind::R11:
  case RBTStateKind::R12:
  case RBTStateKind::R13:
  case RBTStateKind::R14:
  case RBTStateKind::R15:
    assert(false);
    break;
  }
}
} // namespace

namespace Rbt::Insert {

void GoToC0(AllocatorType &Allocator, LogType &log, SMStateType &State,
            uint64_t Key, uint64_t Value) {
  RBTStateKind CurrentState = State.getStateKind();

  assert(CurrentState == RBTStateKind::C0);

  processStateC0(log, State, Key, Value);
  CurrentState = State.getStateKind();

  assert(CurrentState != RBTStateKind::C0);

  while (CurrentState != RBTStateKind::C0) {
    processCurrentState(Allocator, log, State);
    CurrentState = State.getStateKind();
  }
}

void RecoverToC0(AllocatorType &Allocator, LogType &log, SMStateType &State) {
  RBTStateKind CurrentState = State.getStateKind();
  while (CurrentState != RBTStateKind::C0) {
    processCurrentState(Allocator, log, State);
    CurrentState = State.getStateKind();
  }
}

} // namespace Rbt::Insert
