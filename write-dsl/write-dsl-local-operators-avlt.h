#pragma once

template <typename NodeIdx> class BalanceInNode {
  AllocatorType &allocator;
  NodeIdx ni;

public:
  using type = Color;

  BalanceInNode(AllocatorType &allocator, NodeIdx ni)
      : allocator(allocator), ni(ni){};

  int8_t *getAddress() { return &allocator.get(ni.readValue())->Balance; };
  int8_t readValue() { return allocator.get(ni.readValue())->Balance; };
};

template <typename NodeIdx> class NegateBalance {
  NodeIdx ni;

public:
  NegateBalance(NodeIdx ni) : ni(ni){};

  int8_t readValue() {
    int8_t bal = ni.readValue();
    return -bal;
  };
};

template <typename NodeIdx> class UpInNode {
  AllocatorType &allocator;
  NodeIdx ni;

public:
  using type = NodeIndex;

  UpInNode(AllocatorType &allocator, NodeIdx ni)
      : allocator(allocator), ni(ni){};

  NodeIndex *getAddress() { return &allocator.get(ni.readValue())->Up; };
  NodeIndex readValue() { return allocator.get(ni.readValue())->Up; };
};

template <typename NodeIdx> class UpDirInNode {
  AllocatorType &allocator;
  NodeIdx ni;

public:
  using type = Direction;

  UpDirInNode(AllocatorType &allocator, NodeIdx ni)
      : allocator(allocator), ni(ni){};

  Direction *getAddress() { return &allocator.get(ni.readValue())->UpDir; };
  Direction readValue() { return allocator.get(ni.readValue())->UpDir; };
};
