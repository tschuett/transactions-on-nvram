#pragma once

class Value {};

class DirectionValue : public Value {
  Direction dir;

public:
  explicit DirectionValue(Direction dir) : dir(dir) {}

  Direction readValue() { return dir; }

  uint8_t getSize() const { return sizeof(Direction); }
};

class KeyValue : public Value {
  uint64_t Key;

public:
  explicit KeyValue(uint64_t Key) : Key(Key){};

  uint64_t readValue() { return Key; }

  uint8_t getSize() const { return sizeof(uint64_t); }
};

class ValueValue : public Value {
  uint64_t Value;

public:
  explicit ValueValue(uint64_t Value) : Value(Value){};

  uint64_t readValue() { return Value; }

  uint8_t getSize() const { return sizeof(uint64_t); }
};

class NodePtrValue : public Value {
  NodeIndex Ni;

public:
  explicit NodePtrValue(NodeIndex Ni) : Ni(Ni) {}

  NodeIndex readValue() { return Ni; }

  uint8_t getSize() const { return sizeof(NodeIndex); }
};

class Int8Value : public Value {
  int8_t I;

public:
  explicit Int8Value(int8_t I) : I(I) {}

  int8_t readValue() { return I; }

  uint8_t getSize() const { return sizeof(int8_t); }
};

class BooleanValue : public Value {
  bool B;

public:
  explicit BooleanValue(bool B) : B(B) {}

  bool readValue() { return B; }

  bool getSize() const { return sizeof(bool); }
};
