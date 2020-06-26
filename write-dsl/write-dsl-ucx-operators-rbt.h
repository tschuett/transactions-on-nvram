#pragma once

#include "window-kind.h"
#include "write-dsl-ucx-log-rbt.h"

namespace {

template <typename NodeIdx> uint32_t readValue(NodeIdx Ni) {
  return readValue(Ni.readValue());
};

template <>[[maybe_unused]] uint32_t readValue<NodeIndex>(NodeIndex Ni) {
  return Ni.getValue();
};

} // namespace

template <typename NodeIdx> class LeftInNode {
  AllocatorType &Allocator;
  NodeIdx ni;

public:
  using type = NodeIndex;

  LeftInNode(AllocatorType &Allocator, NodeIdx ni)
      : Allocator(Allocator), ni(ni){};

  void writeValue(NodeIndex Ni) { Allocator.writeLeft(ni.readValue(), Ni); }
  NodeIndex readValue() { return Allocator.readLeft(ni.readValue()); }

  uint8_t getSize() const { return sizeof(NodeIndex); }
  uint32_t getOffset() {
    return offsetof(Node, left) + ni.readValue().getValue() * sizeof(Node);
  }
  WindowKind getWindow() const { return WindowKind::NodeWindow; };
};

template <> class LeftInNode<LogAddress::TmpNode> {
  LogType &log;

public:
  using type = NodeIndex;

  LeftInNode(LogType &log) : log(log){};

  NodeIndex readValue() { return log.readLeftInTmpNode(); }

  uint8_t getSize() const { return sizeof(NodeIndex); }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

template <typename NodeIdx> class RightInNode {
  AllocatorType &Allocator;
  NodeIdx ni;

public:
  using type = NodeIndex;

  RightInNode(AllocatorType &Allocator, NodeIdx ni)
      : Allocator(Allocator), ni(ni){};

  void writeValue(NodeIndex Ni) { Allocator.writeRight(ni.readValue(), Ni); }

  NodeIndex readValue() { return Allocator.readRight(ni.readValue()); }

  uint8_t getSize() const { return sizeof(NodeIndex); }
  uint32_t getOffset() {
    return offsetof(Node, right) + ni.readValue().getValue() * sizeof(Node);
  }
  WindowKind getWindow() const { return WindowKind::NodeWindow; };
};

template <> class RightInNode<LogAddress::TmpNode> {
  LogType &log;

public:
  using type = NodeIndex;

  RightInNode(LogType &log) : log(log){};

  NodeIndex readValue() { return log.readRightInTmpNode(); }

  uint8_t getSize() const { return sizeof(NodeIndex); }
  WindowKind getWindow() const { return WindowKind::LogWindow; };
};

template <typename NodeIdx, typename DirectionType> class ChildInNode {
  AllocatorType &Allocator;
  DirectionType dir;
  NodeIdx ni;

public:
  using type = NodeIndex;

  ChildInNode(AllocatorType &Allocator, DirectionType dir, NodeIdx ni)
      : Allocator(Allocator), dir(dir), ni(ni){};

  NodeIndex readValue() {
    Direction Dir = dir.readValue();
    if (Dir == Direction::Left)
      return Allocator.readLeft(ni.readValue());
    else if (Dir == Direction::Right)
      return Allocator.readRight(ni.readValue());
    else
      assert(false);
  }

  void writeValue(NodeIndex Ni) {
    Direction Dir = dir.readValue();
    if (Dir == Direction::Left)
      Allocator.writeLeft(ni.readValue(), Ni);
    else if (Dir == Direction::Right)
      Allocator.writeRight(ni.readValue(), Ni);
    else
      assert(false);
  }

  uint32_t getOffset() {
    Direction Dir = dir.readValue();
    if (Dir == Direction::Left)
      return offsetof(Node, left) + ::readValue<NodeIdx>(ni) * sizeof(Node);
    else if (Dir == Direction::Right)
      return offsetof(Node, right) + ::readValue<NodeIdx>(ni) * sizeof(Node);
    else
      assert(false);
  }

  WindowKind getWindow() const { return WindowKind::NodeWindow; };

  uint8_t getSize() const { return sizeof(NodeIndex); }
};

template <typename NodeIdx> class KeyInNode {
  AllocatorType &Allocator;
  NodeIdx ni;

public:
  using type = uint64_t;

  KeyInNode(AllocatorType &Allocator, NodeIdx ni)
      : Allocator(Allocator), ni(ni){};

  uint64_t readValue() { return Allocator.readKey(ni.readValue()); }

  void writeValue(uint64_t Key) { Allocator.writeKey(ni.readValue(), Key); }

  uint32_t getOffset() {
    return offsetof(Node, key) + ::readValue<NodeIdx>(ni) * sizeof(Node);
  }

  uint8_t getSize() const { return sizeof(uint64_t); }

  WindowKind getWindow() const { return WindowKind::NodeWindow; };
};

template <typename NodeIdx> class ValueInNode {
  AllocatorType &Allocator;
  NodeIdx ni;

public:
  using type = uint64_t;

  ValueInNode(AllocatorType &Allocator, NodeIdx ni)
      : Allocator(Allocator), ni(ni){};

  uint64_t readValue() { return Allocator.readValue(ni.readValue()); }

  void writeValue(uint64_t Value) {
    Allocator.writeValue(ni.readValue(), Value);
  }

  WindowKind getWindow() const { return WindowKind::NodeWindow; };

  uint8_t getSize() const { return sizeof(uint64_t); }

  uint32_t getOffset() {
    return offsetof(Node, value) + ::readValue<NodeIdx>(ni) * sizeof(Node);
  }
};

template <typename NodeIdx> class ColorInNode {
  AllocatorType &allocator;
  NodeIdx ni;

public:
  using type = Color;

  ColorInNode(AllocatorType &allocator, NodeIdx ni)
      : allocator(allocator), ni(ni){};

  void writeValue(Color color) { allocator.writeColor(ni.readValue(), color); }

  Color readValue() { return allocator.readColor(ni.readValue()); }

  uint32_t getOffset() {
    return offsetof(Node, color) + ni.readValue().getValue() * sizeof(Node);
  }
  WindowKind getWindow() const { return WindowKind::NodeWindow; };
};

template <typename NodeIdx> class IsRed {
  AllocatorType &Allocator;
  NodeIdx ni;

public:
  IsRed(AllocatorType &Allocator, NodeIdx ni) : Allocator(Allocator), ni(ni){};

  bool readValue() {
    return not IsNullPtr<NodeIdx>(ni).readValue() and
           ColorInNode<NodeIdx>(Allocator, ni).readValue() == Color::Red;
  };
};
