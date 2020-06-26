#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <string>

enum class Color : uint8_t { Red, Black, Unknown };
enum class Direction : uint8_t { Left, Right, Unknown };

namespace {
using NodeIndexType = uint32_t;
}

class NodeIndex {
  NodeIndexType Index = std::numeric_limits<NodeIndexType>::max();

public:
  constexpr explicit NodeIndex(uint64_t Index) : Index(Index) {}
  NodeIndex() = default;

  constexpr bool operator==(const NodeIndex &rhs) const {
    return Index == rhs.Index;
  }

  constexpr bool operator!=(const NodeIndex &rhs) const {
    return Index != rhs.Index;
  }

  constexpr bool operator<(const NodeIndex &rhs) const { return Index < rhs.Index; }

  constexpr NodeIndexType getValue() const { return Index; }
};

static constexpr NodeIndex Nullptr =
    NodeIndex(std::numeric_limits<NodeIndexType>::max());

class alignas(32) Node {
public:
  uint64_t key = 0;
  uint64_t value = 0;
  NodeIndex Up = Nullptr;
  NodeIndex left = Nullptr;
  NodeIndex right = Nullptr;
  Color color = Color::Red;
  Direction UpDir = Direction::Unknown;
  int8_t Balance = 0;

public:
  void setKey(uint64_t k) { key = k; }
  void setValue(uint64_t v) { value = v; }

  uint64_t getKey() const { return key; }
  NodeIndex getLeft() const { return left; }
  NodeIndex getRight() const { return right; }
  Color getColor() const { return color; }

  NodeIndex &getChild(Direction dir) {
    if (dir == Direction::Left)
      return left;
    else if (dir == Direction::Right)
      return right;
    else {
      assert(false);
      return left;
    }
  }
};

extern std::string Color2String(Color);
extern std::string Direction2String(Direction);
