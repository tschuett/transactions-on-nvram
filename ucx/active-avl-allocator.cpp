#include "active-avl-allocator.h"

#include "avlt-allocator.h"
#include "write-dsl-ucx-avlt.h"

namespace ucx::avlt {

uint64_t ActiveAllocator::readCurrent() {
  uint64_t CurrentValue;

  ucs_status_t status = ucp_get(ep, &CurrentValue, sizeof(uint64_t),
                                BaseAddressCurrentNode, CurrentNodeKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return CurrentValue;
}

void ActiveAllocator::writeCurrent(uint64_t Current) {

  uint64_t TheCurrent = Current;

  ucs_status_t status = ucp_put(ep, &TheCurrent, sizeof(uint64_t),
                                BaseAddressCurrentNode, CurrentNodeKey);
  if (status != UCS_OK)
    printf("failed to put (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  CommonMessage Message;
  Message.tag = Tag::FLUSH;
  Message.Count = 1;
  Message.Flush[0].Window = WindowKind::CurrentNodeWindow;
  Message.Flush[0].Offset = 0;
  Message.Flush[0].Size = sizeof(uint64_t);
  ucx::config::Comm->sendFlushMessage(Message);
}

NodeIndex ActiveAllocator::getNewNodeFromLog() {
  uint64_t key = LogAddress::Key(log).readValue();
  uint64_t value = LogAddress::Value(log).readValue();
  // assert(current < capacity);

  Node node;
  node.key = key;
  node.value = value;
  node.left = Nullptr;
  node.right = Nullptr;

  uint64_t current = readCurrent();

  writeNode(NodeIndex(current), node);

  writeCurrent(current + 1);
  return NodeIndex(current);
}

void ActiveAllocator::writeRight(NodeIndex Ni, NodeIndex Ni2) {
  size_t Offset = Ni.getValue() * sizeof(Node) + offsetof(Node, right);

  NodeIndex TheNi = Ni2;

  ucs_status_t status = ucp_put(ep, &TheNi, sizeof(NodeIndex),
                                BaseAddressNodes + Offset, NodesKey);

  if (status != UCS_OK)
    printf("failed to put (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);
}

void ActiveAllocator::writeLeft(NodeIndex Ni, NodeIndex Ni2) {
  size_t Offset = Ni.getValue() * sizeof(Node) + offsetof(Node, left);

  NodeIndex TheNi = Ni2;

  ucs_status_t status = ucp_put(ep, &TheNi, sizeof(NodeIndex),
                                BaseAddressNodes + Offset, NodesKey);

  if (status != UCS_OK)
    printf("failed to put (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);
}

void ActiveAllocator::writeKey(NodeIndex Ni, uint64_t Key) {
  size_t Offset = Ni.getValue() * sizeof(Node) + offsetof(Node, key);

  uint64_t TheKey = Key;

  ucs_status_t status = ucp_put(ep, &TheKey, sizeof(uint64_t),
                                BaseAddressNodes + Offset, NodesKey);

  if (status != UCS_OK)
    printf("failed to put (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);
}

void ActiveAllocator::writeValue(NodeIndex Ni, uint64_t Value) {
  size_t Offset = Ni.getValue() * sizeof(Node) + offsetof(Node, value);

  uint64_t TheValue = Value;

  ucs_status_t status = ucp_put(ep, &TheValue, sizeof(uint64_t),
                                BaseAddressNodes + Offset, NodesKey);

  if (status != UCS_OK)
    printf("failed to put (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);
}

void ActiveAllocator::writeNode(NodeIndex Ni, Node Nd) {
  size_t Offset = Ni.getValue() * sizeof(Node);

  Node TheNode = Nd;

  ucs_status_t status =
      ucp_put(ep, &TheNode, sizeof(Node), BaseAddressNodes + Offset, NodesKey);

  if (status != UCS_OK)
    printf("failed to put (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);
}

Node ActiveAllocator::readNode(NodeIndex Ni) {
  size_t Offset = Ni.getValue() * sizeof(Node);

  Node NodeValue;

  assert(Ni != Nullptr);

  ucs_status_t status = ucp_get(ep, &NodeValue, sizeof(Node),
                                BaseAddressNodes + Offset, NodesKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return NodeValue;
}

NodeIndex ActiveAllocator::readLeft(NodeIndex Ni) {
  size_t Offset = Ni.getValue() * sizeof(Node) + offsetof(Node, left);

  NodeIndex LeftValue;

  ucs_status_t status = ucp_get(ep, &LeftValue, sizeof(NodeIndex),
                                BaseAddressNodes + Offset, NodesKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return LeftValue;
}

NodeIndex ActiveAllocator::readRight(NodeIndex Ni) {
  size_t Offset = Ni.getValue() * sizeof(Node) + offsetof(Node, right);

  assert(Ni != Nullptr);

  NodeIndex RightValue;

  ucs_status_t status = ucp_get(ep, &RightValue, sizeof(NodeIndex),
                                BaseAddressNodes + Offset, NodesKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return RightValue;
}

uint64_t ActiveAllocator::readKey(NodeIndex Ni) {
  size_t Offset = Ni.getValue() * sizeof(Node) + offsetof(Node, key);

  uint64_t KeyValue;

  ucs_status_t status = ucp_get(ep, &KeyValue, sizeof(uint64_t),
                                BaseAddressNodes + Offset, NodesKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return KeyValue;
}

void ActiveAllocator::writeBalance(NodeIndex Ni, int8_t Val) {
  size_t Offset = Ni.getValue() * sizeof(Node) + offsetof(Node, Balance);

  int8_t TheValue = Val;

  ucs_status_t status = ucp_put(ep, &TheValue, sizeof(int8_t),
                                BaseAddressNodes + Offset, NodesKey);

  if (status != UCS_OK)
    printf("failed to put (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);
}

int8_t ActiveAllocator::readBalance(NodeIndex Ni) {
  size_t Offset = Ni.getValue() * sizeof(Node) + offsetof(Node, Balance);

  int8_t BalanceValue;

  ucs_status_t status = ucp_get(ep, &BalanceValue, sizeof(int8_t),
                                BaseAddressNodes + Offset, NodesKey);
  if (status != UCS_OK)
    printf("failed to get (%s)\n", ucs_status_string(status));
  assert(status == UCS_OK);

  return BalanceValue;
}

} // namespace ucx::avlt
