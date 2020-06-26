#pragma once

#include "window-kind.h"

template <typename Key1, typename Key2> class SmallerThanKey {
  Key1 K1;
  Key2 K2;

public:
  SmallerThanKey(Key1 K1, Key2 K2) : K1(K1), K2(K2){};

  bool readValue() {
    if (K1.readValue() < K2.readValue())
      return true;
    return false;
  }
};

template <typename NodeIdx> class NegateDirection {
  NodeIdx ni;

public:
  NegateDirection(NodeIdx ni) : ni(ni){};

  Direction readValue() {
    Direction dir = ni.readValue();
    if (dir == Direction::Left)
      return Direction::Right;
    if (dir == Direction::Right)
      return Direction::Left;
    assert(false);
    return Direction::Unknown;
  };

  uint8_t getSize() const { return ni.getSize(); }
};

template <typename NodeIdx> class IsNullPtr {
  NodeIdx ni;

public:
  IsNullPtr(NodeIdx ni) : ni(ni){};

  bool readValue() { return ni.readValue() == Nullptr; }
};

template <typename Key1, typename Key2> class EqualKey {
  Key1 K1;
  Key2 K2;

public:
  EqualKey(Key1 K1, Key2 K2) : K1(K1), K2(K2){};

  bool readValue() { return K1.readValue() == K2.readValue(); }
};

template <typename NodeIdx1, typename NodeIdx2> class EqualNodes {
  NodeIdx1 ni1;
  NodeIdx2 ni2;

public:
  EqualNodes(NodeIdx1 ni1, NodeIdx2 ni2) : ni1(ni1), ni2(ni2){};

  bool readValue() { return ni1.readValue() == ni2.readValue(); }
};
