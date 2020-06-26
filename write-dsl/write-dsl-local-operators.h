#pragma once

template <typename NodeIdx, typename DirectionType> class ChildInNode {
  AllocatorType &Allocator;
  DirectionType dir;
  NodeIdx ni;

public:
  using type = NodeIndex;

  ChildInNode(AllocatorType &Allocator, DirectionType dir, NodeIdx ni)
      : Allocator(Allocator), dir(dir), ni(ni){};

  NodeIndex readValue() {
    return Allocator.get(ni.readValue())->getChild(dir.readValue());
  }

  NodeIndex *getAddress() {
    return &Allocator.get(ni.readValue())->getChild(dir.readValue());
  }
};

template <typename NodeIdx> class RightInNode {
  AllocatorType &allocator;
  NodeIdx ni;

public:
  using type = NodeIndex;

  RightInNode(AllocatorType &allocator, NodeIdx ni)
      : allocator(allocator), ni(ni){};

  NodeIndex readValue() { return allocator.get(ni.readValue())->right; }
  NodeIndex *getAddress() { return &allocator.get(ni.readValue())->right; }
};

template <typename NodeIdx> class LeftInNode {
  AllocatorType &allocator;
  NodeIdx ni;

public:
  using type = NodeIndex;

  LeftInNode(AllocatorType &allocator, NodeIdx ni)
      : allocator(allocator), ni(ni){};

  NodeIndex readValue() { return allocator.get(ni.readValue())->left; }
  NodeIndex *getAddress() { return &allocator.get(ni.readValue())->left; }
};

template <typename NodeIdx> class KeyInNode {
  AllocatorType &allocator;
  NodeIdx ni;

public:
  using type = uint64_t;

  KeyInNode(AllocatorType &allocator, NodeIdx ni)
      : allocator(allocator), ni(ni){};

  uint64_t readValue() { return allocator.get(ni.readValue())->key; }
  uint64_t *getAddress() { return &allocator.get(ni.readValue())->key; }
};

template <typename NodeIdx> class ValueInNode {
  AllocatorType &allocator;
  NodeIdx ni;

public:
  using type = uint64_t;

  ValueInNode(AllocatorType &allocator, NodeIdx ni)
      : allocator(allocator), ni(ni){};

  uint64_t readValue() { return allocator.get(ni.readValue())->value; }
  uint64_t *getAddress() { return &allocator.get(ni.readValue())->value; }
};
