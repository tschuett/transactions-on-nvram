#pragma once

#include <cstdint>
#include <string>

enum class AVLTStateKind : uint32_t {
  C0 = 0,
  A1 = 1,
  A2,
  A3,
  A4,
  A5,
  A6,
  A7,
  A8,
  A9,
  A10,
  A11,
  A12,
  A13,
  A14,
  A15,
  A16,
  A17,
  A18,
  A19,
  A20,
  A21,
  A22,
  A23,
  A24,
  A25,
  A26,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  R16,
  R17,
  R18,
  R19,
  R20,
  R21,
  R22,
  R23,
  R24,
  R25,
  R26,
  R27,
  R28,
  R29,
  R30,
  R31,
  R32,
  R33,
  R34
};

enum class RBTStateKind : uint32_t {
  C0 = 0,
  A1 = 1,
  A2,
  A3,
  A4,
  A5,
  A6,
  A7,
  A8,
  A9,
  A10,
  A11,
  A12,
  A13,
  A14,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15
};

struct alignas(8) StateStructure {
  uint32_t EvenOdd;
  RBTStateKind State;

  bool operator==(const StateStructure &Other) {
    return EvenOdd == Other.EvenOdd && State == Other.State;
  }

  bool operator!=(const StateStructure &Other) { return not(*this == Other); }

  uint64_t getAsUint64T() const;
};

struct alignas(8) AVLTStateStructure {
  uint32_t EvenOdd;
  AVLTStateKind State;

  bool operator==(const AVLTStateStructure &Other) {
    return EvenOdd == Other.EvenOdd && State == Other.State;
  }

  bool operator!=(const AVLTStateStructure &Other) {
    return not(*this == Other);
  }

  uint64_t getAsUint64T() const;
};

extern StateStructure StateStructureFromUint64T(uint64_t Int);
extern AVLTStateStructure AVLTStateStructureFromUint64T(uint64_t Int);

extern std::string StateKind2String(RBTStateKind);
extern std::string StateKind2String(AVLTStateKind);
