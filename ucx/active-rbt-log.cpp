#include "active-rbt-log.h"

#include "rbt-log.h"
#include "ucx-config.h"

#include <cstring>

namespace {

void putSyncOrAsync(ucp_ep_h ep, const void *buffer, size_t length,
                    uint64_t remote_addr, ucp_rkey_h rkey) {
  ucs_status_t status;
  if (ucx::config::AsyncPut) {
    status = ucp_put_nbi(ep, buffer, length, remote_addr, rkey);
    assert(status == UCS_OK or status == UCS_INPROGRESS);
  } else {
    status = ucp_put(ep, buffer, length, remote_addr, rkey);
    if (status != UCS_OK)
      printf("failed to put (%s)\n", ucs_status_string(status));
    assert(status == UCS_OK);
  }
}

} // namespace

namespace ucx::rbt {

void ActiveLog::init(ucp_rkey_h LogKey_, uintptr_t BaseAddress_) {
  LogKey = LogKey_;
  BaseAddress = BaseAddress_;
}

void ActiveLog::reset() { memset(&TheLog, 0, sizeof(RBTLogStructure)); }

void ActiveLog::writeKey(uint64_t Key) {
  uint64_t TheKey = Key;

  size_t KeyOffset = offsetof(RBTLogStructure, Key);
  putSyncOrAsync(ep, &TheKey, sizeof(uint64_t), BaseAddress + KeyOffset,
                 LogKey);

  TheLog.Key = Key;
}

void ActiveLog::writeValue(uint64_t Value) {
  uint64_t TheValue = Value;

  size_t ValueOffset = offsetof(RBTLogStructure, Value);

  putSyncOrAsync(ep, &TheValue, sizeof(uint64_t), BaseAddress + ValueOffset,
                 LogKey);

  TheLog.Value = Value;
}

NodeIndex ActiveLog::readRoot() {
  size_t RootOffset = offsetof(RBTLogStructure, Root);

  NodeIndex RootValue;

  ucs_status_t status = ucp_get(ep, &RootValue, sizeof(NodeIndex),
                                BaseAddress + RootOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return RootValue;
}

void ActiveLog::writeDir(uint8_t EvenOdd, Direction Dir) {
  Direction TheDir = Dir;

  size_t DirOffset = offsetof(RBTLogStructure, Iterator) +
                     EvenOdd * sizeof(Iterators) + offsetof(Iterators, Dir);

  putSyncOrAsync(ep, &TheDir, sizeof(Direction), BaseAddress + DirOffset,
                 LogKey);

  TheLog.Iterator[EvenOdd].Dir = Dir;
}

void ActiveLog::writeDir2(Direction Dir) {
  Direction TheDir2 = Dir;

  size_t Dir2Offset = offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Dir2);
  putSyncOrAsync(ep, &TheDir2, sizeof(Direction), BaseAddress + Dir2Offset,
                 LogKey);

  TheLog.Log.Dir2 = Dir;
}

void ActiveLog::writeDir3(Direction Dir) {
  Direction TheDir3 = Dir;

  size_t Dir3Offset = offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Dir3);
  putSyncOrAsync(ep, &TheDir3, sizeof(Direction), BaseAddress + Dir3Offset,
                 LogKey);

  TheLog.Log.Dir3 = Dir;
}

void ActiveLog::writeSp(NodeIndex ni) {
  NodeIndex TheNi = ni;

  size_t SpOffset = offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Sp);
  putSyncOrAsync(ep, &TheNi, sizeof(NodeIndex), BaseAddress + SpOffset, LogKey);

  TheLog.Log.Sp = ni;
}

void ActiveLog::writeSaveP(NodeIndex ni) {
  NodeIndex TheNi = ni;

  size_t SavePOffset =
      offsetof(RBTLogStructure, Log) + offsetof(RedoLog, SaveP);
  putSyncOrAsync(ep, &TheNi, sizeof(NodeIndex), BaseAddress + SavePOffset,
                 LogKey);

  TheLog.Log.SaveP = ni;
}

void ActiveLog::writeSaveChildP(NodeIndex ni) {
  NodeIndex TheNi = ni;

  size_t SaveChildPOffset =
      offsetof(RBTLogStructure, Log) + offsetof(RedoLog, SaveChildP);
  putSyncOrAsync(ep, &TheNi, sizeof(NodeIndex), BaseAddress + SaveChildPOffset,
                 LogKey);

  TheLog.Log.SaveChildP = ni;
}

void ActiveLog::writeRoot2P(NodeIndex ni) {
  NodeIndex TheNi = ni;

  size_t Root2POffset =
      offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Root2P);
  putSyncOrAsync(ep, &TheNi, sizeof(NodeIndex), BaseAddress + Root2POffset,
                 LogKey);

  TheLog.Log.Root2P = ni;
}

void ActiveLog::writeParent2P(NodeIndex ni) {
  NodeIndex TheNi = ni;

  size_t Parent2POffset =
      offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Parent2P);
  putSyncOrAsync(ep, &TheNi, sizeof(NodeIndex), BaseAddress + Parent2POffset,
                 LogKey);

  TheLog.Log.Parent2P = ni;
}

void ActiveLog::writeTp(uint8_t EvenOdd, NodeIndex ni) {
  NodeIndex TheTp = ni;

  printf("writeTp: %u\n", ni.getValue());

  size_t TpOffset = offsetof(RBTLogStructure, Iterator) +
                    EvenOdd * sizeof(Iterators) + offsetof(Iterators, Tp);

  putSyncOrAsync(ep, &TheTp, sizeof(NodeIndex), BaseAddress + TpOffset, LogKey);

  TheLog.Iterator[EvenOdd].Tp = ni;
}

void ActiveLog::writeGrandP(uint8_t EvenOdd, NodeIndex ni) {
  NodeIndex TheGrandP = ni;

  size_t GrandPOffset = offsetof(RBTLogStructure, Iterator) +
                        EvenOdd * sizeof(Iterators) +
                        offsetof(Iterators, GrandP);

  putSyncOrAsync(ep, &TheGrandP, sizeof(NodeIndex), BaseAddress + GrandPOffset,
                 LogKey);

  TheLog.Iterator[EvenOdd].GrandP = ni;
}

void ActiveLog::writeParentP(uint8_t EvenOdd, NodeIndex ni) {
  NodeIndex TheParentP = ni;

  size_t ParentPOffset = offsetof(RBTLogStructure, Iterator) +
                         EvenOdd * sizeof(Iterators) +
                         offsetof(Iterators, ParentP);

  putSyncOrAsync(ep, &TheParentP, sizeof(NodeIndex),
                 BaseAddress + ParentPOffset, LogKey);

  TheLog.Iterator[EvenOdd].ParentP = ni;
}

void ActiveLog::writeQP(uint8_t EvenOdd, NodeIndex ni) {
  NodeIndex TheQP = ni;

  // printf("writeQP %d\n", ni.getValue());

  size_t QPOffset = offsetof(RBTLogStructure, Iterator) +
                    EvenOdd * sizeof(Iterators) + offsetof(Iterators, QP);

  putSyncOrAsync(ep, &TheQP, sizeof(NodeIndex), BaseAddress + QPOffset, LogKey);

  TheLog.Iterator[EvenOdd].QP = ni;
}

void ActiveLog::writeQ2P(uint8_t EvenOdd, NodeIndex ni) {
  NodeIndex TheQ2P = ni;

  size_t Q2POffset = offsetof(RBTLogStructure, Iterator) +
                     EvenOdd * sizeof(Iterators) + offsetof(Iterators, Q2P);

  putSyncOrAsync(ep, &TheQ2P, sizeof(NodeIndex), BaseAddress + Q2POffset,
                 LogKey);

  TheLog.Iterator[EvenOdd].Q2P = ni;
}

NodeIndex ActiveLog::readQP(uint8_t EvenOdd) {
  if (ucx::config::CacheLog)
    return TheLog.Iterator[EvenOdd].QP;

  size_t QPOffset = offsetof(RBTLogStructure, Iterator) +
                    EvenOdd * sizeof(Iterators) + offsetof(Iterators, QP);

  NodeIndex QPValue;

  // printf("readQP: %d %zu %lu\n", EvenOdd, QPOffset, sizeof(LogStructure));
  ucs_status_t status =
      ucp_get(ep, &QPValue, sizeof(NodeIndex), BaseAddress + QPOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  // ucs_status_ptr_t status =
  //    ucp_get_nb(ep, &QPValue, sizeof(NodeIndex), BaseAddress + QPOffset,
  //               LogKey, empty_send_cb);
  //
  // waitRequest(worker, status);

  return QPValue;
}

NodeIndex ActiveLog::readQ2P(uint8_t EvenOdd) {
  if (ucx::config::CacheLog)
    return TheLog.Iterator[EvenOdd].Q2P;

  size_t Q2POffset = offsetof(RBTLogStructure, Iterator) +
                     EvenOdd * sizeof(Iterators) + offsetof(Iterators, Q2P);

  NodeIndex Q2PValue;

  ucs_status_t status = ucp_get(ep, &Q2PValue, sizeof(NodeIndex),
                                BaseAddress + Q2POffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return Q2PValue;
}

void ActiveLog::writeTmpNode(Node N) {
  Node TheTmpNode = N;

  size_t TmpNodeOffset =
      offsetof(RBTLogStructure, Log) + offsetof(RedoLog, TmpNode);
  putSyncOrAsync(ep, &TheTmpNode, sizeof(Node), BaseAddress + TmpNodeOffset,
                 LogKey);

  TheLog.Log.TmpNode = N;
}

NodeIndex ActiveLog::readLeftInTmpNode() {
  if (ucx::config::CacheLog)
    return TheLog.Log.TmpNode.left;

  size_t Offset = offsetof(RBTLogStructure, Log) + offsetof(RedoLog, TmpNode) +
                  offsetof(Node, left);

  NodeIndex LeftInTmpNodeValue;

  ucs_status_t status = ucp_get(ep, &LeftInTmpNodeValue, sizeof(NodeIndex),
                                BaseAddress + Offset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return LeftInTmpNodeValue;
}

NodeIndex ActiveLog::readRightInTmpNode() {
  if (ucx::config::CacheLog)
    return TheLog.Log.TmpNode.right;

  size_t Offset = offsetof(RBTLogStructure, Log) + offsetof(RedoLog, TmpNode) +
                  offsetof(Node, right);

  NodeIndex RightInTmpNodeValue;

  ucs_status_t status = ucp_get(ep, &RightInTmpNodeValue, sizeof(NodeIndex),
                                BaseAddress + Offset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return RightInTmpNodeValue;
}

uint64_t ActiveLog::readKey() {
  if (ucx::config::CacheLog)
    return TheLog.Key;

  size_t KeyOffset = offsetof(RBTLogStructure, Key);

  uint64_t KeyValue;

  ucs_status_t status =
      ucp_get(ep, &KeyValue, sizeof(uint64_t), BaseAddress + KeyOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return KeyValue;
}

uint64_t ActiveLog::readValue() {
  if (ucx::config::CacheLog)
    return TheLog.Value;

  size_t ValueOffset = offsetof(RBTLogStructure, Value);

  uint64_t ValueValue;

  ucs_status_t status = ucp_get(ep, &ValueValue, sizeof(uint64_t),
                                BaseAddress + ValueOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return ValueValue;
}

void ActiveLog::writeRoot(NodeIndex ni) {
  NodeIndex TheRoot = ni;

  size_t RootOffset = offsetof(RBTLogStructure, Root);
  putSyncOrAsync(ep, &TheRoot, sizeof(NodeIndex), BaseAddress + RootOffset,
                 LogKey);

  TheLog.Root = ni;
}

NodeIndex ActiveLog::readGrandP(uint8_t EvenOdd) {
  if (ucx::config::CacheLog)
    return TheLog.Iterator[EvenOdd].GrandP;

  size_t GrandPOffset = offsetof(RBTLogStructure, Iterator) +
                        EvenOdd * sizeof(Iterators) +
                        offsetof(Iterators, GrandP);

  NodeIndex GrandPValue;

  ucs_status_t status = ucp_get(ep, &GrandPValue, sizeof(NodeIndex),
                                BaseAddress + GrandPOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return GrandPValue;
}

Direction ActiveLog::readDir(uint8_t EvenOdd) {
  if (ucx::config::CacheLog)
    return TheLog.Iterator[EvenOdd].Dir;

  size_t DirOffset = offsetof(RBTLogStructure, Iterator) +
                     EvenOdd * sizeof(Iterators) + offsetof(Iterators, Dir);

  Direction DirValue;

  ucs_status_t status = ucp_get(ep, &DirValue, sizeof(NodeIndex),
                                BaseAddress + DirOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return DirValue;
}

Direction ActiveLog::readDir2() {
  if (ucx::config::CacheLog)
    return TheLog.Log.Dir2;

  size_t Dir2Offset = offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Dir2);

  Direction Dir2Value;

  ucs_status_t status = ucp_get(ep, &Dir2Value, sizeof(NodeIndex),
                                BaseAddress + Dir2Offset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return Dir2Value;
}

NodeIndex ActiveLog::readSp() {
  if (ucx::config::CacheLog)
    return TheLog.Log.Sp;

  size_t SpOffset = offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Sp);

  NodeIndex SpValue;

  ucs_status_t status =
      ucp_get(ep, &SpValue, sizeof(NodeIndex), BaseAddress + SpOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return SpValue;
}

NodeIndex ActiveLog::readParent2P() {
  if (ucx::config::CacheLog)
    return TheLog.Log.Parent2P;

  size_t Parent2POffset =
      offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Parent2P);

  NodeIndex Parent2PValue;

  ucs_status_t status = ucp_get(ep, &Parent2PValue, sizeof(NodeIndex),
                                BaseAddress + Parent2POffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return Parent2PValue;
}

Direction ActiveLog::readLastDir(uint8_t EvenOdd) {
  if (ucx::config::CacheLog)
    return TheLog.Iterator[EvenOdd].LastDir;

  size_t LastDirOffset = offsetof(RBTLogStructure, Iterator) +
                         EvenOdd * sizeof(Iterators) +
                         offsetof(Iterators, LastDir);

  Direction LastDirValue;

  ucs_status_t status = ucp_get(ep, &LastDirValue, sizeof(NodeIndex),
                                BaseAddress + LastDirOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return LastDirValue;
}

void ActiveLog::writeLastDir(uint8_t EvenOdd, Direction Dir) {
  size_t LastDirOffset = offsetof(RBTLogStructure, Iterator) +
                         EvenOdd * sizeof(Iterators) +
                         offsetof(Iterators, LastDir);
  Direction TheLastDir = Dir;

  putSyncOrAsync(ep, &TheLastDir, sizeof(Direction),
                 BaseAddress + LastDirOffset, LogKey);

  TheLog.Iterator[EvenOdd].LastDir = Dir;
}

NodeIndex ActiveLog::readParentP(uint8_t EvenOdd) {
  if (ucx::config::CacheLog)
    return TheLog.Iterator[EvenOdd].ParentP;

  size_t ParentPOffset = offsetof(RBTLogStructure, Iterator) +
                         EvenOdd * sizeof(Iterators) +
                         offsetof(Iterators, ParentP);

  NodeIndex ParentPValue;

  ucs_status_t status = ucp_get(ep, &ParentPValue, sizeof(NodeIndex),
                                BaseAddress + ParentPOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return ParentPValue;
}

NodeIndex ActiveLog::readSaveP() {
  if (ucx::config::CacheLog)
    return TheLog.Log.SaveP;

  size_t SavePOffset =
      offsetof(RBTLogStructure, Log) + offsetof(RedoLog, SaveP);

  NodeIndex SavePValue;

  ucs_status_t status = ucp_get(ep, &SavePValue, sizeof(NodeIndex),
                                BaseAddress + SavePOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return SavePValue;
}

NodeIndex ActiveLog::readSaveChildP() {
  if (ucx::config::CacheLog)
    return TheLog.Log.SaveChildP;

  size_t SaveChildPOffset =
      offsetof(RBTLogStructure, Log) + offsetof(RedoLog, SaveChildP);

  NodeIndex SaveChildPValue;

  ucs_status_t status = ucp_get(ep, &SaveChildPValue, sizeof(NodeIndex),
                                BaseAddress + SaveChildPOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return SaveChildPValue;
}

NodeIndex ActiveLog::readFp(uint8_t EvenOdd) {
  if (ucx::config::CacheLog)
    return TheLog.Iterator[EvenOdd].Fp;

  size_t FpOffset = offsetof(RBTLogStructure, Iterator) +
                    EvenOdd * sizeof(Iterators) + offsetof(Iterators, Fp);

  NodeIndex FpValue;

  ucs_status_t status =
      ucp_get(ep, &FpValue, sizeof(NodeIndex), BaseAddress + FpOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return FpValue;
}

NodeIndex ActiveLog::readRoot2P() {
  if (ucx::config::CacheLog)
    return TheLog.Log.Root2P;

  size_t Root2POffset =
      offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Root2P);

  NodeIndex Root2PValue;

  ucs_status_t status = ucp_get(ep, &Root2PValue, sizeof(NodeIndex),
                                BaseAddress + Root2POffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return Root2PValue;
}

Direction ActiveLog::readDir3() {
  if (ucx::config::CacheLog)
    return TheLog.Log.Dir3;

  size_t Dir3Offset = offsetof(RBTLogStructure, Log) + offsetof(RedoLog, Dir3);

  Direction Dir3Value;

  ucs_status_t status = ucp_get(ep, &Dir3Value, sizeof(NodeIndex),
                                BaseAddress + Dir3Offset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return Dir3Value;
}

Iterators ActiveLog::readIterators(uint8_t EvenOdd) {
  if (ucx::config::CacheLog)
    return TheLog.Iterator[EvenOdd];

  size_t ItsOffset =
      offsetof(RBTLogStructure, Iterator) + EvenOdd * sizeof(Iterators);

  Iterators ItsValue;

  ucs_status_t status = ucp_get(ep, &ItsValue, sizeof(Iterators),
                                BaseAddress + ItsOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  // printf("read iterators: %d %u\n", EvenOdd, ItsValue.Tp.getValue());

  return ItsValue;
}

NodeIndex ActiveLog::readTp(uint8_t EvenOdd) {
  if (ucx::config::CacheLog)
    return TheLog.Iterator[EvenOdd].Tp;

  size_t TpOffset = offsetof(RBTLogStructure, Iterator) +
                    EvenOdd * sizeof(Iterators) + offsetof(Iterators, Tp);

  NodeIndex TpValue;

  ucs_status_t status =
      ucp_get(ep, &TpValue, sizeof(NodeIndex), BaseAddress + TpOffset, LogKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  // printf("read Tp: %d %u\n", EvenOdd, TpValue.getValue());

  return TpValue;
}

void ActiveLog::writeIterators(uint8_t EvenOdd, Iterators Its) {
  size_t ItsOffset =
      offsetof(RBTLogStructure, Iterator) + EvenOdd * sizeof(Iterators);
  Iterators TheIts = Its;

  // printf("write iterators: %u\n", Its.Tp.getValue());

  putSyncOrAsync(ep, &TheIts, sizeof(Iterators), BaseAddress + ItsOffset,
                 LogKey);

  TheLog.Iterator[EvenOdd] = Its;
}

} // namespace ucx::rbt
