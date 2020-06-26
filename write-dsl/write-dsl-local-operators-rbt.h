#pragma once

#include "write-dsl-local-log-rbt.h"

template <typename NodeIdx> class ColorInNode {
  AllocatorType &allocator;
  NodeIdx ni;

public:
  using type = Color;

  ColorInNode(AllocatorType &allocator, NodeIdx ni)
      : allocator(allocator), ni(ni){};

  Color *getAddress() { return &allocator.get(ni.readValue())->color; };
  Color readValue() { return allocator.get(ni.readValue())->color; };
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

template <> class RightInNode<LogAddress::TmpNode> {
  LogType &log;

public:
  using type = NodeIndex;

  RightInNode(LogType &log) : log(log){};

  NodeIndex readValue() {
    LogAddress::TmpNode tmp = {log};
    return log.getLog()->Log.TmpNode.right;
  }
};

template <> class LeftInNode<LogAddress::TmpNode> {
  LogType &log;

public:
  using type = NodeIndex;

  LeftInNode(LogType &log) : log(log){};

  NodeIndex readValue() {
    LogAddress::TmpNode tmp = {log};
    return log.getLog()->Log.TmpNode.left;
  }
};

