#include "state-machine-remove.h"

#include "rbt-allocator.h"
#include "flush.h"
#include "rbt-log.h"
#include "plot.h"
#include "recover.h"
#include "state-kind.h"
#include "tree.h"
#include "write-dsl-rbt.h"

#include <cstdint>

namespace {

// initialize variables
void transitionC0ToDX(AllocatorType &Allocator, LogType &log, uint64_t Key,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Iterators Its;
  Its.LastDir = Direction::Unknown;
  Its.Dir = Direction::Right;
  Its.Tp = Nullptr;
  Its.GrandP = Nullptr;
  Its.ParentP = Nullptr;
  Its.QP = Allocator.getHeadNode();
  Its.Q2P = Allocator.getHeadNode(); // billiger hack
  Its.Fp = Nullptr;

  WriteOp<LogAddress::Key, KeyValue> W1 = {LogAddress::Key(log), KeyValue(Key)};

  WriteOp<LogAddress::Iterators, IteratorsValue> W2 = {
      LogAddress::Iterators(log, EvenOdd), IteratorsValue(Its)};

  WriteOp<RightInNode<LogAddress::QP>, LogAddress::Root> W3 = {
      RightInNode<LogAddress::QP>(Allocator, LogAddress::QP(log, EvenOdd)),
      LogAddress::Root(log)};

  // flush(
  //     [&] {
  //       write(&Log->Iterator[EvenOdd].Fp, Nullptr);
  //       write(&Log->Key, Key);
  //       write(&Log->Iterator[EvenOdd].Dir, Direction::Right);
  //       write(&Log->Iterator[EvenOdd].QP, Allocator.getHeadNode());
  //       write(&Log->Iterator[EvenOdd].GrandP, Nullptr);
  //       write(&Log->Iterator[EvenOdd].ParentP, Nullptr);
  //       write(&Allocator.get(Log->Iterator[EvenOdd].QP)->right, Log->Root);
  //     },
  //     FlushKind::InitializeVariables);

  flushOp(FlushKind::InitializeVariables, W1, W2);
}

// update helpers
void transitionR1ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();
  uint32_t NextEvenOdd = State.getFlippedEvenOddBit();

  NodeIndex NewQP = ChildInNode<LogAddress::QP, LogAddress::Dir>(
                        Allocator, LogAddress::Dir(log, EvenOdd),
                        LogAddress::QP(log, EvenOdd))
                        .readValue();

  Direction NextDir = Direction::Unknown;

  if (SmallerThanKey<KeyInNode<NodePtrValue>, LogAddress::Key>(
          KeyInNode<NodePtrValue>(Allocator, NodePtrValue(NewQP)),
          LogAddress::Key(log))
          .readValue())
    NextDir = Direction::Right;
  else
    NextDir = Direction::Left;

  Iterators Its;

  Its.LastDir = LogAddress::Dir(log, EvenOdd).readValue();
  Its.Dir = NextDir;
  Its.Tp = Nullptr;
  Its.GrandP = LogAddress::ParentP(log, EvenOdd).readValue();
  Its.ParentP = LogAddress::QP(log, EvenOdd).readValue();
  Its.QP = NewQP;
  Its.Q2P = NewQP;

  if (EqualNodes<KeyInNode<NodePtrValue>, LogAddress::Key>(
          KeyInNode<NodePtrValue>(Allocator, NodePtrValue(NewQP)),
          LogAddress::Key(log))
      .readValue()) {
    Its.Fp = NewQP;
  } else {
    Its.Fp = LogAddress::Fp(log, EvenOdd).readValue();
  }

  WriteOp<LogAddress::Iterators, IteratorsValue> W1 = {
      LogAddress::Iterators(log, NextEvenOdd), IteratorsValue(Its)};

  flushOp(FlushKind::ShuffleIterators, W1);
}

// flush before: replace and remove if found
void transitionR13ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction tmpDir = Direction::Unknown;
  if (IsNullPtr<LogAddress::QP>(LogAddress::QP(log, EvenOdd)).readValue())
    tmpDir = Direction::Right;
  else
    tmpDir = Direction::Left;

  Direction Dir3 = Direction::Unknown;
  if (EqualNodes<RightInNode<LogAddress::ParentP>, LogAddress::QP>(
          RightInNode<LogAddress::ParentP>(Allocator,
                                           LogAddress::ParentP(log, EvenOdd)),
          LogAddress::QP(log, EvenOdd))
          .readValue())
    Dir3 = Direction::Left;
  else
    Dir3 = Direction::Right;

  // assert(Log->Iterator[EvenOdd].Fp != Nullptr);

  // flush(
  //    [&] {
  //      write(&Log->Log.Dir2, tmpDir);
  //      write(&Log->Log.Dir3, Dir3);
  //    },
  //    FlushKind::RemoveNode);

  WriteOp<LogAddress::Dir2, DirectionValue> W1 = {LogAddress::Dir2(log),
                                                  DirectionValue(tmpDir)};

  WriteOp<LogAddress::Dir3, DirectionValue> W2 = {LogAddress::Dir3(log),
                                                  DirectionValue(Dir3)};

  flushOp(FlushKind::RemoveNode, W1, W2);
}

// replace and remove if found
void transitionR14ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction tmpDir = LogAddress::Dir2(log).readValue();
  Direction Dir3 = LogAddress::Dir3(log).readValue();

  //  Requires logging ? ? ?

  if (Dir3 == Direction::Left) {
    // flush(
    //    [&] {
    //      write(&Allocator.get(Log->Iterator[EvenOdd].Fp)->key,
    //            Allocator.get(Log->Iterator[EvenOdd].QP)->key);
    //      write(&Allocator.get(Log->Iterator[EvenOdd].Fp)->value,
    //            Allocator.get(Log->Iterator[EvenOdd].QP)->value);
    //
    //      write(&Allocator.get(Log->Iterator[EvenOdd].ParentP)->right,
    //            Allocator.get(Log->Iterator[EvenOdd].QP)->getChild(tmpDir));
    //    },
    //    FlushKind::RemoveNode);

    WriteOp<KeyInNode<LogAddress::Fp>, KeyInNode<LogAddress::QP>> W1 = {
        KeyInNode<LogAddress::Fp>(Allocator, LogAddress::Fp(log, EvenOdd)),
        KeyInNode<LogAddress::QP>(Allocator, LogAddress::QP(log, EvenOdd))};

    WriteOp<ValueInNode<LogAddress::Fp>, ValueInNode<LogAddress::QP>> W2 = {
        ValueInNode<LogAddress::Fp>(Allocator, LogAddress::Fp(log, EvenOdd)),
        ValueInNode<LogAddress::QP>(Allocator, LogAddress::QP(log, EvenOdd))};

    WriteOp<RightInNode<LogAddress::ParentP>,
            ChildInNode<LogAddress::QP, DirectionValue>>
        W3 = {RightInNode<LogAddress::ParentP>(
                  Allocator, LogAddress::ParentP(log, EvenOdd)),
              ChildInNode<LogAddress::QP, DirectionValue>(
                  Allocator, DirectionValue(tmpDir),
                  LogAddress::QP(log, EvenOdd))};

    flushOp(FlushKind::RemoveNode, W1, W2, W3);

  } else if (Dir3 == Direction::Right) {
    WriteOp<KeyInNode<LogAddress::Fp>, KeyInNode<LogAddress::QP>> W1 = {
        KeyInNode<LogAddress::Fp>(Allocator, LogAddress::Fp(log, EvenOdd)),
        KeyInNode<LogAddress::QP>(Allocator, LogAddress::QP(log, EvenOdd))};

    WriteOp<ValueInNode<LogAddress::Fp>, ValueInNode<LogAddress::QP>> W2 = {
        ValueInNode<LogAddress::Fp>(Allocator, LogAddress::Fp(log, EvenOdd)),
        ValueInNode<LogAddress::QP>(Allocator, LogAddress::QP(log, EvenOdd))};

    WriteOp<LeftInNode<LogAddress::ParentP>,
            ChildInNode<LogAddress::QP, DirectionValue>>
        W3 = {LeftInNode<LogAddress::ParentP>(
                  Allocator, LogAddress::ParentP(log, EvenOdd)),
              ChildInNode<LogAddress::QP, DirectionValue>(
                  Allocator, DirectionValue(tmpDir),
                  LogAddress::QP(log, EvenOdd))};

    flushOp(FlushKind::RemoveNode, W1, W2, W3);

    //    flush(
    //        [&] {
    //          write(&Allocator.get(Log->Iterator[EvenOdd].Fp)->key,
    //                Allocator.get(Log->Iterator[EvenOdd].QP)->key);
    //          write(&Allocator.get(Log->Iterator[EvenOdd].Fp)->value,
    //                Allocator.get(Log->Iterator[EvenOdd].QP)->value);
    //
    //          write(&Allocator.get(Log->Iterator[EvenOdd].ParentP)->left,
    //                Allocator.get(Log->Iterator[EvenOdd].QP)->getChild(tmpDir));
    //        },
    //        FlushKind::RemoveNode);
  } else
    assert(false);
}

// update root and make it black
void transitionR15ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {

  NodeIndex NewRoot = RightInNode<NodePtrValue>(
                          Allocator, NodePtrValue(Allocator.getHeadNode()))
                          .readValue();

  WriteOp<LogAddress::Root, NodePtrValue> W1 = {LogAddress::Root(log),
                                                NodePtrValue(NewRoot)};

  if (NewRoot != Nullptr) {

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W2 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(NewRoot)),
        ColorValue(Color::Black)};

    flushOp(FlushKind::UpdateRoot, W1, W2);

    // flush(
    //    [&] {
    //      write(&Log->Root, NewRoot);
    //      write(&Allocator.get(NewRoot)->color, Color::Black);
    //    },
    //    FlushKind::UpdateRoot);
  } else {

    flushOp(FlushKind::UpdateRoot, W1);

    // flush([&] { write(&Log->Root, NewRoot); }, FlushKind::UpdateRoot);
  }
}

// flush for the first single rotation
void transitionR2ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction dir = LogAddress::Dir(log, EvenOdd).readValue();
  NodeIndex tmp = Nullptr;
  NodeIndex save = Nullptr;

  if (dir == Direction::Left)
    save = RightInNode<LogAddress::QP>(Allocator, LogAddress::QP(log, EvenOdd))
               .readValue();
  else if (dir == Direction::Right)
    save = LeftInNode<LogAddress::QP>(Allocator, LogAddress::QP(log, EvenOdd))
               .readValue();
  else
    assert(false);

  if (dir == Direction::Left)
    tmp = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(save)).readValue();
  else if (dir == Direction::Right)
    tmp = RightInNode<NodePtrValue>(Allocator, NodePtrValue(save)).readValue();
  else
    assert(false);

  // flush(
  //    [&] {
  //      write(&Log->Log.SaveP, save);
  //      write(&Log->Log.Parent2P, Log->Iterator[EvenOdd].ParentP);
  //      write(&Log->Log.SaveChildP, tmp);
  //    },
  //    FlushKind::SingleRotation);

  WriteOp<LogAddress::SaveP, NodePtrValue> W1 = {LogAddress::SaveP(log),
                                                 NodePtrValue(save)};

  WriteOp<LogAddress::Parent2P, LogAddress::ParentP> W2 = {
      LogAddress::Parent2P(log), LogAddress::ParentP(log, EvenOdd)};

  WriteOp<LogAddress::SaveChildP, NodePtrValue> W3 = {
      LogAddress::SaveChildP(log), NodePtrValue(tmp)};

  flushOp(FlushKind::SingleRotation, W1, W2, W3);
}

// first single rotation
void transitionR3ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  // Direction dir = Log->Iterator[EvenOdd].Dir;
  Direction dir = LogAddress::Dir(log, EvenOdd).readValue();

  NodeIndex Save = LogAddress::SaveP(log).readValue();
  NodeIndex root = LogAddress::QP(log, EvenOdd).readValue();
  NodeIndex SaveChild = LogAddress::SaveChildP(log).readValue();

  // if (isRecover) {
  //   printf("R3: parent=%d save=%d\n", log_->parentp.getValue(),
  //   Save.getValue());
  // }

  assert(Save != Nullptr);

  if (dir == Direction::Left) {
    // flush(
    //    [&] {
    //      write(&Allocator.get(root)->right, SaveChild);
    //      write(&Allocator.get(Save)->left, root);
    //      write(&Allocator.get(root)->color, Color::Red);
    //      write(&Allocator.get(Save)->color, Color::Black);
    //      write(&Log->Iterator[EvenOdd].ParentP, Save);
    //      write(&Allocator.get(Log->Log.Parent2P)
    //                 ->getChild(Log->Iterator[EvenOdd].LastDir),
    //            Save);
    //    },
    //    FlushKind::SingleRotation);

    WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
        NodePtrValue(SaveChild)};

    WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        NodePtrValue(root)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
        ColorValue(Color::Red)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        ColorValue(Color::Black)};

    WriteOp<LogAddress::ParentP, NodePtrValue> W5 = {
        LogAddress::ParentP(log, EvenOdd), NodePtrValue(Save)};

    WriteOp<ChildInNode<LogAddress::Parent2P, LogAddress::LastDir>,
            NodePtrValue>
        W6 = {ChildInNode<LogAddress::Parent2P, LogAddress::LastDir>(
                  Allocator, LogAddress::LastDir(log, EvenOdd),
                  LogAddress::Parent2P(log)),
              NodePtrValue(Save)};

    flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5, W6);

  } else if (dir == Direction::Right) {
    // flush(
    //    [&] {
    //      write(&Allocator.get(root)->left, SaveChild);
    //      write(&Allocator.get(Save)->right, root);
    //      write(&Allocator.get(root)->color, Color::Red);
    //      write(&Allocator.get(Save)->color, Color::Black);
    //      write(&Log->Iterator[EvenOdd].ParentP, Save);
    //      write(&Allocator.get(Log->Log.Parent2P)
    //                 ->getChild(Log->Iterator[EvenOdd].LastDir),
    //            Save);
    //    },
    //    FlushKind::SingleRotation);

    WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
        NodePtrValue(SaveChild)};

    WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        NodePtrValue(root)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
        ColorValue(Color::Red)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        ColorValue(Color::Black)};

    WriteOp<LogAddress::ParentP, NodePtrValue> W5 = {
        LogAddress::ParentP(log, EvenOdd), NodePtrValue(Save)};

    WriteOp<ChildInNode<LogAddress::Parent2P, LogAddress::LastDir>,
            NodePtrValue>
        W6 = {ChildInNode<LogAddress::Parent2P, LogAddress::LastDir>(
                  Allocator, LogAddress::LastDir(log, EvenOdd),
                  LogAddress::Parent2P(log)),
              NodePtrValue(Save)};

    flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5, W6);
  }
}

// flush S
void transitionR4ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction dir2 = Direction::Unknown;

  if (LogAddress::GrandP(log, EvenOdd).readValue() != Nullptr) {
    if (EqualNodes<RightInNode<LogAddress::GrandP>, LogAddress::ParentP>(
            RightInNode<LogAddress::GrandP>(Allocator,
                                            LogAddress::GrandP(log, EvenOdd)),
            LogAddress::ParentP(log, EvenOdd))
            .readValue())
      dir2 = Direction::Right;
    else
      dir2 = Direction::Left;
  }

  // flush(
  //    [&] {
  //      write(&Log->Log.Sp,
  //            Allocator.get(Log->Iterator[EvenOdd].ParentP)
  //                ->getChild(negate(Log->Iterator[EvenOdd].LastDir)));
  //      write(&Log->Log.Dir2, dir2);
  //    },
  //    FlushKind::Misc);

  WriteOp<LogAddress::Sp, ChildInNode<LogAddress::ParentP,
                                      NegateDirection<LogAddress::LastDir>>>
      W1 = {LogAddress::Sp(log),
            ChildInNode<LogAddress::ParentP,
                        NegateDirection<LogAddress::LastDir>>(
                Allocator, NegateDirection<LogAddress::LastDir>(LogAddress::LastDir(log, EvenOdd)),
                LogAddress::ParentP(log, EvenOdd))};

  WriteOp<LogAddress::Dir2, DirectionValue> W2 = {LogAddress::Dir2(log),
                                                  DirectionValue(dir2)};

  flushOp(FlushKind::Misc, W1, W2);
}

// color flip
void transitionR5ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  // assert(Log->Iterator[EvenOdd].ParentP != Nullptr);
  // assert(Log->Log.sp != Nullptr);
  // assert(Log->Iterator[EvenOdd].QP != Nullptr);

  // flush(
  //    [&] {
  //      write(&Allocator.get(Log->Iterator[EvenOdd].ParentP)->color,
  //            Color::Black);
  //      write(&Allocator.get(Log->Log.Sp)->color, Color::Red);
  //      write(&Allocator.get(Log->Iterator[EvenOdd].QP)->color, Color::Red);
  //    },
  //    FlushKind::ColorFlip);

  WriteOp<ColorInNode<LogAddress::ParentP>, ColorValue> W1 = {
      ColorInNode<LogAddress::ParentP>(Allocator,
                                       LogAddress::ParentP(log, EvenOdd)),
      ColorValue(Color::Black)};

  WriteOp<ColorInNode<LogAddress::Sp>, ColorValue> W2 = {
      ColorInNode<LogAddress::Sp>(Allocator, LogAddress::Sp(log)),
      ColorValue(Color::Red)};

  WriteOp<ColorInNode<LogAddress::QP>, ColorValue> W3 = {
      ColorInNode<LogAddress::QP>(Allocator, LogAddress::QP(log, EvenOdd)),
      ColorValue(Color::Red)};

  flushOp(FlushKind::ColorFlip, W1, W2, W3);
}

// flush to disk for the first single roation in double rotation
void transitionR8ToDX(AllocatorType &Allocator, LogType &log,
                      SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction dir2 = Direction::Unknown;
  Direction dir =
      NegateDirection<LogAddress::LastDir>(LogAddress::LastDir(log, EvenOdd))
          .readValue();
  NodeIndex root = Nullptr;
  NodeIndex Save = Nullptr;
  NodeIndex SaveChild = Nullptr;

  if (EqualNodes<RightInNode<LogAddress::GrandP>, LogAddress::ParentP>(
          RightInNode<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd)),
          LogAddress::ParentP(log, EvenOdd))
          .readValue())
    dir2 = Direction::Right;
  else
    dir2 = Direction::Left;

  if (dir == Direction::Left)
    root = LeftInNode<LogAddress::ParentP>(Allocator,
                                           LogAddress::ParentP(log, EvenOdd))
               .readValue();
  else if (dir == Direction::Right)
    root = RightInNode<LogAddress::ParentP>(Allocator,
                                            LogAddress::ParentP(log, EvenOdd))
               .readValue();
  else
    assert(false);

  if (dir == Direction::Left)
    Save = RightInNode<NodePtrValue>(Allocator, NodePtrValue(root)).readValue();
  else if (dir == Direction::Right)
    Save = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(root)).readValue();
  else
    assert(false);

  if (dir == Direction::Left)
    SaveChild =
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
  else if (dir == Direction::Right)
    SaveChild =
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();

  // flush(
  //    [&] {
  //      write(&Log->Log.Dir2, dir2); // needed in second rotation
  //      write(&Log->Log.Root2P, root);
  //      write(&Log->Log.SaveP, Save);
  //      write(&Log->Log.SaveChildP, SaveChild);
  //    },
  //    FlushKind::DoubleRotation);

  WriteOp<LogAddress::Dir2, DirectionValue> W1 = {LogAddress::Dir2(log),
                                                  DirectionValue(dir2)};

  WriteOp<LogAddress::Root2P, NodePtrValue> W2 = {LogAddress::Root2P(log),
                                                  NodePtrValue(root)};

  WriteOp<LogAddress::SaveP, NodePtrValue> W3 = {LogAddress::SaveP(log),
                                                 NodePtrValue(Save)};

  WriteOp<LogAddress::SaveChildP, NodePtrValue> W4 = {
      LogAddress::SaveChildP(log), NodePtrValue(SaveChild)};

  flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4);
}

// first single rotation in double rotation
void transitionR9ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction dir =
      NegateDirection<LogAddress::LastDir>(LogAddress::LastDir(log, EvenOdd))
          .readValue();

  NodeIndex root = LogAddress::Root2P(log).readValue();
  NodeIndex Save = LogAddress::SaveP(log).readValue();
  NodeIndex SaveChild = LogAddress::SaveChildP(log).readValue();

  if (dir == Direction::Left) {
    // flush(
    //    [&] {
    //      write(&Allocator.get(root)->right, SaveChild);
    //      write(&Allocator.get(Save)->left, root);
    //      write(&Allocator.get(root)->color, Color::Red);
    //      write(&Allocator.get(Save)->color, Color::Black);
    //      write(&Allocator.get(Log->Iterator[EvenOdd].ParentP)->left, Save);
    //    },
    //    FlushKind::DoubleRotation);

    WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
        NodePtrValue(SaveChild)};

    WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        NodePtrValue(root)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
        ColorValue(Color::Red)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        ColorValue(Color::Black)};

    WriteOp<LeftInNode<LogAddress::ParentP>, NodePtrValue> W5 = {
        LeftInNode<LogAddress::ParentP>(Allocator,
                                        LogAddress::ParentP(log, EvenOdd)),
        NodePtrValue(Save)};

    flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);

  } else if (dir == Direction::Right) {
    // flush(
    //    [&] {
    //      write(&Allocator.get(root)->left, SaveChild);
    //      write(&Allocator.get(Save)->right, root);
    //      write(&Allocator.get(root)->color, Color::Red);
    //      write(&Allocator.get(Save)->color, Color::Black);
    //      write(&Allocator.get(Log->Iterator[EvenOdd].ParentP)->right, Save);
    //    },
    //    FlushKind::DoubleRotation);

    WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
        NodePtrValue(SaveChild)};

    WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        NodePtrValue(root)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
        ColorValue(Color::Red)};

    WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
        ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
        ColorValue(Color::Black)};

    WriteOp<RightInNode<LogAddress::ParentP>, NodePtrValue> W5 = {
        RightInNode<LogAddress::ParentP>(Allocator,
                                         LogAddress::ParentP(log, EvenOdd)),
        NodePtrValue(Save)};

    flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);
  }
}

// flush to disk for the second single roation in double rotation
void transitionR10ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction dir = LogAddress::LastDir(log, EvenOdd).readValue();
  NodeIndex root = LogAddress::ParentP(log, EvenOdd).readValue();
  NodeIndex Save = Nullptr;
  NodeIndex SaveChild = Nullptr;

  if (dir == Direction::Left)
    Save = RightInNode<NodePtrValue>(Allocator, NodePtrValue(root)).readValue();
  else if (dir == Direction::Right)
    Save = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(root)).readValue();
  else
    assert(false);

  if (dir == Direction::Left)
    SaveChild =
        LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();
  else if (dir == Direction::Right)
    SaveChild =
        RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)).readValue();

  // flush(
  //    [&] {
  //      write(&Log->Log.SaveP, Save);
  //      write(&Log->Log.SaveChildP, SaveChild);
  //    },
  //    FlushKind::DoubleRotation);

  WriteOp<LogAddress::SaveP, NodePtrValue> W1 = {LogAddress::SaveP(log),
                                                 NodePtrValue(Save)};

  WriteOp<LogAddress::SaveChildP, NodePtrValue> W2 = {
      LogAddress::SaveChildP(log), NodePtrValue(SaveChild)};

  flushOp(FlushKind::DoubleRotation, W1, W2);
}

// second single rotation in double rotation
void transitionR11ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction dir = LogAddress::LastDir(log, EvenOdd).readValue();
  Direction dir2 = LogAddress::Dir2(log).readValue();

  NodeIndex Save = LogAddress::SaveP(log).readValue();
  NodeIndex root = LogAddress::ParentP(log, EvenOdd).readValue();
  NodeIndex SaveChild = LogAddress::SaveChildP(log).readValue();

  assert(root != Nullptr);
  assert(Save != Nullptr);
  // assert(SaveChild != Nullptr);

  if (dir == Direction::Left) {
    if (dir2 == Direction::Left) {
      // flush(
      //    [&] {
      //      write(&Allocator.get(root)->right, SaveChild);
      //      write(&Allocator.get(Save)->left, root);
      //      write(&Allocator.get(root)->color, Color::Red);
      //      write(&Allocator.get(Save)->color, Color::Black);
      //      write(&Allocator.get(Log->Iterator[EvenOdd].GrandP)->left, Save);
      //    },
      //    FlushKind::DoubleRotation);

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          NodePtrValue(SaveChild)};

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<LeftInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
          LeftInNode<LogAddress::GrandP>(Allocator,
                                         LogAddress::GrandP(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);

    } else {
      // flush(
      //    [&] {
      //      write(&Allocator.get(root)->right, SaveChild);
      //      write(&Allocator.get(Save)->left, root);
      //      write(&Allocator.get(root)->color, Color::Red);
      //      write(&Allocator.get(Save)->color, Color::Black);
      //      write(&Allocator.get(Log->Iterator[EvenOdd].GrandP)->right, Save);
      //    },
      //    FlushKind::DoubleRotation);

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          NodePtrValue(SaveChild)};

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<RightInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
          RightInNode<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);
    }
  } else if (dir == Direction::Right) {
    if (dir2 == Direction::Left) {
      // flush(
      //    [&] {
      //      write(&Allocator.get(root)->left, SaveChild);
      //      write(&Allocator.get(Save)->right, root);
      //      write(&Allocator.get(root)->color, Color::Red);
      //      write(&Allocator.get(Save)->color, Color::Black);
      //      write(&Allocator.get(Log->Iterator[EvenOdd].GrandP)->left, Save);
      //    },
      //    FlushKind::DoubleRotation);

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          NodePtrValue(SaveChild)};

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<LeftInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
          LeftInNode<LogAddress::GrandP>(Allocator,
                                         LogAddress::GrandP(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);

    } else {
      // flush(
      //    [&] {
      //      write(&Allocator.get(root)->left, SaveChild);
      //      write(&Allocator.get(Save)->right, root);
      //      write(&Allocator.get(root)->color, Color::Red);
      //      write(&Allocator.get(Save)->color, Color::Black);
      //      write(&Allocator.get(Log->Iterator[EvenOdd].GrandP)->right, Save);
      //    },
      //    FlushKind::DoubleRotation);

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          NodePtrValue(SaveChild)};

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<RightInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
          RightInNode<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::DoubleRotation, W1, W2, W3, W4, W5);
    }
  }
}

// flush to disk for the second single rotation
void transitionR6ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction dir = LogAddress::LastDir(log, EvenOdd).readValue();
  Direction dir2 = Direction::Unknown;
  NodeIndex tmp = Nullptr;
  NodeIndex save = Nullptr;

  if (EqualNodes<RightInNode<LogAddress::GrandP>, LogAddress::ParentP>(
          RightInNode<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd)),
          LogAddress::ParentP(log, EvenOdd))
          .readValue())
    dir2 = Direction::Right;
  else
    dir2 = Direction::Left;

  if (dir == Direction::Left)
    save = RightInNode<LogAddress::ParentP>(Allocator,
                                            LogAddress::ParentP(log, EvenOdd))
               .readValue();
  else if (dir == Direction::Right)
    save = LeftInNode<LogAddress::ParentP>(Allocator,
                                           LogAddress::ParentP(log, EvenOdd))
               .readValue();
  else
    assert(false);

  if (dir == Direction::Left)
    tmp = LeftInNode<NodePtrValue>(Allocator, NodePtrValue(save)).readValue();
  else if (dir == Direction::Right)
    tmp = RightInNode<NodePtrValue>(Allocator, NodePtrValue(save)).readValue();
  else
    assert(false);

  // flush(
  //    [&] {
  //      write(&Log->Log.SaveChildP, tmp);
  //      write(&Log->Log.SaveP, save);
  //      write(&Log->Log.Dir2, dir2);
  //    },
  //    FlushKind::SingleRotation);

  WriteOp<LogAddress::SaveChildP, NodePtrValue> W1 = {
      LogAddress::SaveChildP(log), NodePtrValue(tmp)};

  WriteOp<LogAddress::SaveP, NodePtrValue> W2 = {LogAddress::SaveP(log),
                                                 NodePtrValue(save)};

  WriteOp<LogAddress::Dir2, DirectionValue> W3 = {LogAddress::Dir2(log),
                                                  DirectionValue(dir2)};

  flushOp(FlushKind::SingleRotation, W1, W2, W3);
}

// second single roation
void transitionR7ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  Direction dir = LogAddress::LastDir(log, EvenOdd).readValue();
  Direction dir2 = LogAddress::Dir2(log).readValue();

  NodeIndex root = LogAddress::ParentP(log, EvenOdd).readValue();
  NodeIndex Save = LogAddress::SaveP(log).readValue();
  NodeIndex SaveChild = LogAddress::SaveChildP(log).readValue();

  assert(Save != Nullptr);

  if (dir == Direction::Left) {
    if (dir2 == Direction::Left) {
      // flush(
      //    [&] {
      //      write(&Allocator.get(root)->right, SaveChild);
      //      write(&Allocator.get(Save)->left, root);
      //      write(&Allocator.get(root)->color, Color::Red);
      //      write(&Allocator.get(Save)->color, Color::Black);
      //      write(&Allocator.get(Log->Iterator[EvenOdd].GrandP)->left, Save);
      //    },
      //    FlushKind::SingleRotation);

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          NodePtrValue(SaveChild)};

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<LeftInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
          LeftInNode<LogAddress::GrandP>(Allocator,
                                         LogAddress::GrandP(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5);

    } else {
      //  flush(
      //      [&] {
      //        write(&Allocator.get(root)->right, SaveChild);
      //        write(&Allocator.get(Save)->left, root);
      //        write(&Allocator.get(root)->color, Color::Red);
      //        write(&Allocator.get(Save)->color, Color::Black);
      //        write(&Allocator.get(Log->Iterator[EvenOdd].GrandP)->right,
      //        Save);
      //      },
      //      FlushKind::SingleRotation);

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W1 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          NodePtrValue(SaveChild)};

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W2 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<RightInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
          RightInNode<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5);
    }
  } else if (dir == Direction::Right) {
    if (dir2 == Direction::Left) {
      // flush(
      //    [&] {
      //      write(&Allocator.get(root)->left, SaveChild);
      //      write(&Allocator.get(Save)->right, root);
      //      write(&Allocator.get(root)->color, Color::Red);
      //      write(&Allocator.get(Save)->color, Color::Black);
      //      write(&Allocator.get(Log->Iterator[EvenOdd].GrandP)->left, Save);
      //    },
      //    FlushKind::SingleRotation);

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          NodePtrValue(SaveChild)};

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<LeftInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
          LeftInNode<LogAddress::GrandP>(Allocator,
                                         LogAddress::GrandP(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5);

    } else {
      // flush(
      //    [&] {
      //      write(&Allocator.get(root)->left, SaveChild);
      //      write(&Allocator.get(Save)->right, root);
      //      write(&Allocator.get(root)->color, Color::Red);
      //      write(&Allocator.get(Save)->color, Color::Black);
      //      write(&Allocator.get(Log->Iterator[EvenOdd].GrandP)->right, Save);
      //    },
      //    FlushKind::SingleRotation);

      WriteOp<LeftInNode<NodePtrValue>, NodePtrValue> W1 = {
          LeftInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          NodePtrValue(SaveChild)};

      WriteOp<RightInNode<NodePtrValue>, NodePtrValue> W2 = {
          RightInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          NodePtrValue(root)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W3 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(root)),
          ColorValue(Color::Red)};

      WriteOp<ColorInNode<NodePtrValue>, ColorValue> W4 = {
          ColorInNode<NodePtrValue>(Allocator, NodePtrValue(Save)),
          ColorValue(Color::Black)};

      WriteOp<RightInNode<LogAddress::GrandP>, NodePtrValue> W5 = {
          RightInNode<LogAddress::GrandP>(Allocator,
                                          LogAddress::GrandP(log, EvenOdd)),
          NodePtrValue(Save)};

      flushOp(FlushKind::SingleRotation, W1, W2, W3, W4, W5);
    }
  }
}

// ensure correct coloring
void transitionR12ToDX(AllocatorType &Allocator, LogType &log,
                       SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  // Node *tmp = Allocator.get(
  //    Allocator.get(Log->Iterator[EvenOdd].GrandP)->getChild(Log->Log.Dir2));

  NodeIndex tmp =
      ChildInNode<LogAddress::GrandP, LogAddress::Dir2>(
          Allocator, LogAddress::Dir2(log), LogAddress::GrandP(log, EvenOdd))
          .readValue();

  // flush(
  //    [&] {
  //      write(&Allocator.get(Log->Iterator[EvenOdd].QP)->color, Color::Red);
  //      write(&Allocator
  //                 .get(Allocator.get(Log->Iterator[EvenOdd].GrandP)
  //                          ->getChild(Log->Log.Dir2))
  //                 ->color,
  //            Color::Red);
  //      write(&Allocator.get(tmp->left)->color, Color::Black);
  //      write(&Allocator.get(tmp->right)->color, Color::Black);
  //    },
  //    FlushKind::Recolor);

  WriteOp<ColorInNode<LogAddress::QP>, ColorValue> W1 = {
      ColorInNode<LogAddress::QP>(Allocator, LogAddress::QP(log, EvenOdd)),
      ColorValue(Color::Red)};

  WriteOp<ColorInNode<ChildInNode<LogAddress::GrandP, LogAddress::Dir2>>,
          ColorValue>
      W2 = {
          ColorInNode<ChildInNode<LogAddress::GrandP, LogAddress::Dir2>>(
              ColorInNode<ChildInNode<LogAddress::GrandP, LogAddress::Dir2>>(
                  Allocator, ChildInNode<LogAddress::GrandP, LogAddress::Dir2>(
                                 Allocator, LogAddress::Dir2(log),
                                 LogAddress::GrandP(log, EvenOdd)))),
          ColorValue(Color::Red)};

  WriteOp<ColorInNode<LeftInNode<NodePtrValue>>, ColorValue> W3 = {
      ColorInNode<LeftInNode<NodePtrValue>>(
          Allocator, LeftInNode<NodePtrValue>(Allocator, NodePtrValue(tmp))),
      ColorValue(Color::Black)};

  WriteOp<ColorInNode<RightInNode<NodePtrValue>>, ColorValue> W4 = {
      ColorInNode<RightInNode<NodePtrValue>>(
          Allocator, RightInNode<NodePtrValue>(Allocator, NodePtrValue(tmp))),
      ColorValue(Color::Black)};

  flushOp(FlushKind::Recolor, W1, W2, W3, W4);
}

// initialize variables
void processStateC0(AllocatorType &Allocator, LogType &log, SMStateType &State,
                    uint64_t Key) {
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionC0ToDX(Allocator, log, Key, State);

  if (!IsNullPtr<ChildInNode<LogAddress::QP, LogAddress::Dir>>(
           ChildInNode<LogAddress::QP, LogAddress::Dir>(
               Allocator, LogAddress::Dir(log, EvenOdd),
               LogAddress::QP(log, EvenOdd)))
           .readValue()) {
    State.changeWoEvenOdd(RBTStateKind::C0, RBTStateKind::R1);
  } else if (LogAddress::Fp(log, EvenOdd).readValue() != Nullptr) {
    assert(false);
    State.changeWoEvenOdd(RBTStateKind::C0, RBTStateKind::R13);
  } else {
    assert(false);
    State.changeWoEvenOdd(RBTStateKind::C0, RBTStateKind::R15);
  }
}

// update helpers
void processStateR1(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  uint32_t NextEvenOdd = State.getFlippedEvenOddBit();

  transitionR1ToDX(Allocator, log, State);
  if (!IsRed<LogAddress::QP>(Allocator, LogAddress::QP(log, NextEvenOdd))
           .readValue() and
      !IsRed<ChildInNode<LogAddress::QP, LogAddress::Dir>>(
           Allocator, ChildInNode<LogAddress::QP, LogAddress::Dir>(
                          Allocator, LogAddress::Dir(log, NextEvenOdd),
                          LogAddress::QP(log, NextEvenOdd)))
           .readValue()) {
    if (IsRed<ChildInNode<LogAddress::QP, NegateDirection<LogAddress::Dir>>>(
            Allocator,
            ChildInNode<LogAddress::QP, NegateDirection<LogAddress::Dir>>(
                Allocator,
                NegateDirection<LogAddress::Dir>(
                    LogAddress::Dir(log, NextEvenOdd)),
                LogAddress::QP(log, NextEvenOdd)))
            .readValue()) {
      State.changeWithEvenOdd(RBTStateKind::R1, RBTStateKind::R2);
    } else if (!IsRed<ChildInNode<LogAddress::QP, LogAddress::Dir>>(
                    Allocator, ChildInNode<LogAddress::QP, LogAddress::Dir>(
                                   Allocator, LogAddress::Dir(log, NextEvenOdd),
                                   LogAddress::QP(log, NextEvenOdd)))
                    .readValue()) {
      State.changeWithEvenOdd(RBTStateKind::R1, RBTStateKind::R4);
    } else if (ChildInNode<LogAddress::QP, LogAddress::Dir>(
                   Allocator, LogAddress::Dir(log, NextEvenOdd),
                   LogAddress::QP(log, NextEvenOdd))
                   .readValue() != Nullptr) {
      State.changeWithEvenOddSame(RBTStateKind::R1, RBTStateKind::R1);
    } else if (LogAddress::Fp(log, NextEvenOdd).readValue() != Nullptr) {
      State.changeWithEvenOdd(RBTStateKind::R1, RBTStateKind::R13);
    } else {
      assert(false);
      State.changeWithEvenOdd(RBTStateKind::R1, RBTStateKind::R15);
    }
  } else if (ChildInNode<LogAddress::QP, LogAddress::Dir>(
                 Allocator, LogAddress::Dir(log, NextEvenOdd),
                 LogAddress::QP(log, NextEvenOdd))
                 .readValue() != Nullptr) {
    State.changeWithEvenOddSame(RBTStateKind::R1, RBTStateKind::R1);
  } else if (LogAddress::Fp(log, NextEvenOdd).readValue() != Nullptr) {
    State.changeWithEvenOdd(RBTStateKind::R1, RBTStateKind::R13);
  } else {
    assert(false);
    State.changeWithEvenOdd(RBTStateKind::R1, RBTStateKind::R15);
  }
}

// replace and remove if found
void processStateR14(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionR14ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R14, RBTStateKind::R15);
}

// update root and make it black
void processStateR15(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionR15ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R15, RBTStateKind::C0);
}

// flush to disk for first single rotation
void processStateR2(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionR2ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R2, RBTStateKind::R3);
}

// first single rotation
void processStateR3(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionR3ToDX(Allocator, log, State);
  if (ChildInNode<LogAddress::QP, LogAddress::Dir>(
          Allocator, LogAddress::Dir(log, EvenOdd),
          LogAddress::QP(log, EvenOdd))
          .readValue() != Nullptr)
    State.changeWoEvenOdd(RBTStateKind::R3, RBTStateKind::R1);
  else if (LogAddress::Fp(log, EvenOdd).readValue() != Nullptr)
    State.changeWoEvenOdd(RBTStateKind::R3, RBTStateKind::R13);
  else {
    assert(false);
    State.changeWoEvenOdd(RBTStateKind::R3, RBTStateKind::R15);
  }
}

// flush s
void processStateR4(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionR4ToDX(Allocator, log, State);
  if (LogAddress::Sp(log).readValue() != Nullptr) {
    if (!IsRed<
             ChildInNode<LogAddress::Sp, NegateDirection<LogAddress::LastDir>>>(
             Allocator,
             ChildInNode<LogAddress::Sp, NegateDirection<LogAddress::LastDir>>(
                 Allocator,
                 NegateDirection<LogAddress::LastDir>(
                     LogAddress::LastDir(log, EvenOdd)),
                 LogAddress::Sp(log)))
             .readValue() and
        !IsRed<ChildInNode<LogAddress::Sp, LogAddress::LastDir>>(
             Allocator, ChildInNode<LogAddress::Sp, LogAddress::LastDir>(
                            Allocator, LogAddress::LastDir(log, EvenOdd),
                            LogAddress::Sp(log)))
             .readValue())
      State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R5);
    else if ((LogAddress::GrandP(log, EvenOdd).readValue() != Nullptr) and
             not(LogAddress::GrandP(log, EvenOdd).readValue() ==
                     Allocator.getHeadNode() and
                 LogAddress::Dir2(log).readValue() ==
                     Direction::Left)) { // double deviation from
                                         // original code
      if (IsRed<ChildInNode<LogAddress::Sp, LogAddress::LastDir>>(
              Allocator, ChildInNode<LogAddress::Sp, LogAddress::LastDir>(
                             Allocator, LogAddress::LastDir(log, EvenOdd),
                             LogAddress::Sp(log)))
              .readValue()) {
        State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R8);

      } else if (IsRed<ChildInNode<LogAddress::Sp,
                                   NegateDirection<LogAddress::LastDir>>>(
                     Allocator,
                     ChildInNode<LogAddress::Sp,
                                 NegateDirection<LogAddress::LastDir>>(
                         Allocator,
                         NegateDirection<LogAddress::LastDir>(LogAddress::LastDir(log, EvenOdd)),
                         LogAddress::Sp(log)))
                     .readValue()) {
        State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R6);
      } else
        State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R12);
    } else {
      if (ChildInNode<LogAddress::QP, LogAddress::Dir>(
              Allocator, LogAddress::Dir(log, EvenOdd),
              LogAddress::QP(log, EvenOdd))
              .readValue() != Nullptr)
        State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R1);
      else if (LogAddress::Fp(log, EvenOdd).readValue() != Nullptr)
        State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R13);
      else {
        assert(false);
        State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R15);
      }
    }
  } else {
    if (ChildInNode<LogAddress::QP, LogAddress::Dir>(
            Allocator, LogAddress::Dir(log, EvenOdd),
            LogAddress::QP(log, EvenOdd))
            .readValue() != Nullptr)
      State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R1);
    else if (LogAddress::Fp(log, EvenOdd).readValue() != Nullptr)
      State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R13);
    else {
      assert(false);
      State.changeWoEvenOdd(RBTStateKind::R4, RBTStateKind::R15);
    }
  }
}

// color flip
void processStateR5(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionR5ToDX(Allocator, log, State);
  if (ChildInNode<LogAddress::QP, LogAddress::Dir>(
          Allocator, LogAddress::Dir(log, EvenOdd),
          LogAddress::QP(log, EvenOdd))
          .readValue() != Nullptr)
    State.changeWoEvenOdd(RBTStateKind::R5, RBTStateKind::R1);
  else if (LogAddress::Fp(log, EvenOdd).readValue() != Nullptr)
    State.changeWoEvenOdd(RBTStateKind::R5, RBTStateKind::R13);
  else {
    assert(false);
    State.changeWoEvenOdd(RBTStateKind::R5, RBTStateKind::R15);
  }
}

// flush to disk for first single rotation in double rotation
void processStateR8(AllocatorType &Allocator, LogType &log,
                    SMStateType &State) {
  transitionR8ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R8, RBTStateKind::R9);
}

// first single rotation in double rotation
void processStateR9(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionR9ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R9, RBTStateKind::R10);
}

// flush to disk for second single rotation in double rotation
void processStateR10(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionR10ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R10, RBTStateKind::R11);
}

// second single rotation in double rotation
void processStateR11(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionR11ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R11, RBTStateKind::R12);
}

// flush to disk for second single rotation
void processStateR6(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionR6ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R6, RBTStateKind::R7);
}

// second single rotation
void processStateR7(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionR7ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R7, RBTStateKind::R12);
}

// ensure correct coloring
void processStateR12(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  uint32_t EvenOdd = State.getEvenOddBit();

  transitionR12ToDX(Allocator, log, State);
  if (ChildInNode<LogAddress::QP, LogAddress::Dir>(
          Allocator, LogAddress::Dir(log, EvenOdd),
          LogAddress::QP(log, EvenOdd))
          .readValue() != Nullptr)
    State.changeWoEvenOdd(RBTStateKind::R12, RBTStateKind::R1);
  else if (LogAddress::Fp(log, EvenOdd).readValue() != Nullptr)
    State.changeWoEvenOdd(RBTStateKind::R12, RBTStateKind::R13);
  else {
    assert(false);
    State.changeWoEvenOdd(RBTStateKind::R12, RBTStateKind::R15);
  }
}

// flush to disk before replace and remove if found
void processStateR13(AllocatorType &Allocator, LogType &log,
                     SMStateType &State) {
  transitionR13ToDX(Allocator, log, State);
  State.changeWoEvenOdd(RBTStateKind::R13, RBTStateKind::R14);
}

void processCurrentState(AllocatorType &Allocator, LogType &log,
                         SMStateType &State) {
  RBTStateKind CurrentState = State.getStateKind();
  switch (CurrentState) {
  case RBTStateKind::C0:
    assert(false);
    break;
  case RBTStateKind::R1:
    processStateR1(Allocator, log, State);
    break;
  case RBTStateKind::R2:
    processStateR2(Allocator, log, State);
    break;
  case RBTStateKind::R3:
    processStateR3(Allocator, log, State);
    break;
  case RBTStateKind::R4:
    processStateR4(Allocator, log, State);
    break;
  case RBTStateKind::R5:
    processStateR5(Allocator, log, State);
    break;
  case RBTStateKind::R6:
    processStateR6(Allocator, log, State);
    break;
  case RBTStateKind::R7:
    processStateR7(Allocator, log, State);
    break;
  case RBTStateKind::R8:
    processStateR8(Allocator, log, State);
    break;
  case RBTStateKind::R9:
    processStateR9(Allocator, log, State);
    break;
  case RBTStateKind::R10:
    processStateR10(Allocator, log, State);
    break;
  case RBTStateKind::R11:
    processStateR11(Allocator, log, State);
    break;
  case RBTStateKind::R12:
    processStateR12(Allocator, log, State);
    break;
  case RBTStateKind::R13:
    processStateR13(Allocator, log, State);
    break;
  case RBTStateKind::R14:
    processStateR14(Allocator, log, State);
    break;
  case RBTStateKind::R15:
    processStateR15(Allocator, log, State);
    break;
  case RBTStateKind::A1:
  case RBTStateKind::A2:
  case RBTStateKind::A3:
  case RBTStateKind::A4:
  case RBTStateKind::A5:
  case RBTStateKind::A6:
  case RBTStateKind::A7:
  case RBTStateKind::A8:
  case RBTStateKind::A9:
  case RBTStateKind::A10:
  case RBTStateKind::A11:
  case RBTStateKind::A12:
  case RBTStateKind::A13:
  case RBTStateKind::A14:
    assert(false);
    break;
  }
}
} // namespace

namespace Rbt::Remove {

void GoToC0(AllocatorType &Allocator, LogType &log, SMStateType &State,
            uint64_t Key) {
  RBTStateKind CurrentState = State.getStateKind();

  assert(CurrentState == RBTStateKind::C0);

  processStateC0(Allocator, log, State, Key);
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
} // namespace Rbt::Remove

