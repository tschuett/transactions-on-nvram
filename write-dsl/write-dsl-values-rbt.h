#pragma once

class ColorValue : public Value {
  Color c;

public:
  ColorValue(Color c) : c(c) {}

  Color readValue() { return c; }

  uint8_t getSize() const { return sizeof(Color); }
};

class IteratorsValue : public Value {
  Iterators Its;

public:
  IteratorsValue(Iterators Its) : Its(Its) {}

  Iterators readValue() { return Its; }

  uint8_t getSize() const { return sizeof(Iterators); }
};
