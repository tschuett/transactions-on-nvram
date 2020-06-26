#include "state-kind.h"

#include <string>

std::string StateKind2String(RBTStateKind Kind) {
  switch (Kind) {
  case RBTStateKind::C0:
    return "C0";
  case RBTStateKind::A1:
    return "A1";
  case RBTStateKind::A2:
    return "A2";
  case RBTStateKind::A3:
    return "A3";
  case RBTStateKind::A4:
    return "A4";
  case RBTStateKind::A5:
    return "A5";
  case RBTStateKind::A6:
    return "A6";
  case RBTStateKind::A7:
    return "A7";
  case RBTStateKind::A8:
    return "A8";
  case RBTStateKind::A9:
    return "A9";
  case RBTStateKind::A10:
    return "A10";
  case RBTStateKind::A11:
    return "A11";
  case RBTStateKind::A12:
    return "A12";
  case RBTStateKind::A13:
    return "A13";
  case RBTStateKind::A14:
    return "A14";

  case RBTStateKind::R1:
    return "R1";
  case RBTStateKind::R2:
    return "R2";
  case RBTStateKind::R3:
    return "R3";
  case RBTStateKind::R4:
    return "R4";
  case RBTStateKind::R5:
    return "R5";
  case RBTStateKind::R6:
    return "R6";
  case RBTStateKind::R7:
    return "R7";
  case RBTStateKind::R8:
    return "R8";
  case RBTStateKind::R9:
    return "R9";
  case RBTStateKind::R10:
    return "R10";
  case RBTStateKind::R11:
    return "R11";
  case RBTStateKind::R12:
    return "R12";
  case RBTStateKind::R13:
    return "R13";
  case RBTStateKind::R14:
    return "R14";
  case RBTStateKind::R15:
    return "R15";
  }
}

std::string StateKind2String(AVLTStateKind Kind) {
  switch (Kind) {
  case AVLTStateKind::C0:
    return "C0";
  case AVLTStateKind::A1:
    return "A1";
  case AVLTStateKind::A2:
    return "A2";
  case AVLTStateKind::A3:
    return "A3";
  case AVLTStateKind::A4:
    return "A4";
  case AVLTStateKind::A5:
    return "A5";
  case AVLTStateKind::A6:
    return "A6";
  case AVLTStateKind::A7:
    return "A7";
  case AVLTStateKind::A8:
    return "A8";
  case AVLTStateKind::A9:
    return "A9";
  case AVLTStateKind::A10:
    return "A10";
  case AVLTStateKind::A11:
    return "A11";
  case AVLTStateKind::A12:
    return "A12";
  case AVLTStateKind::A13:
    return "A13";
  case AVLTStateKind::A14:
    return "A14";
  case AVLTStateKind::A15:
    return "A15";
  case AVLTStateKind::A16:
    return "A16";
  case AVLTStateKind::A17:
    return "A17";
  case AVLTStateKind::A18:
    return "A18";
  case AVLTStateKind::A19:
    return "A19";
  case AVLTStateKind::A20:
    return "A20";
  case AVLTStateKind::A21:
    return "A21";
  case AVLTStateKind::A22:
    return "A22";
  case AVLTStateKind::A23:
    return "A23";
  case AVLTStateKind::A24:
    return "A24";
  case AVLTStateKind::A25:
    return "A25";
  case AVLTStateKind::A26:
    return "A26";
  case AVLTStateKind::R1:
    return "R1";
  case AVLTStateKind::R2:
    return "R2";
  case AVLTStateKind::R3:
    return "R3";
  case AVLTStateKind::R4:
    return "R4";
  case AVLTStateKind::R5:
    return "R5";
  case AVLTStateKind::R6:
    return "R6";
  case AVLTStateKind::R7:
    return "R7";
  case AVLTStateKind::R8:
    return "R8";
  case AVLTStateKind::R9:
    return "R9";
  case AVLTStateKind::R10:
    return "R10";
  case AVLTStateKind::R11:
    return "R11";
  case AVLTStateKind::R12:
    return "R12";
  case AVLTStateKind::R13:
    return "R13";
  case AVLTStateKind::R14:
    return "R14";
  case AVLTStateKind::R15:
    return "R15";
  case AVLTStateKind::R16:
    return "R16";
  case AVLTStateKind::R17:
    return "R17";
  case AVLTStateKind::R18:
    return "R18";
  case AVLTStateKind::R19:
    return "R19";
  case AVLTStateKind::R20:
    return "R20";
  case AVLTStateKind::R21:
    return "R21";
  case AVLTStateKind::R22:
    return "R22";
  case AVLTStateKind::R23:
    return "R23";
  case AVLTStateKind::R24:
    return "R24";
  case AVLTStateKind::R25:
    return "R25";
  case AVLTStateKind::R26:
    return "R26";
  case AVLTStateKind::R27:
    return "R27";
  case AVLTStateKind::R28:
    return "R28";
  case AVLTStateKind::R29:
    return "R29";
  case AVLTStateKind::R30:
    return "R30";
  case AVLTStateKind::R31:
    return "R31";
  case AVLTStateKind::R32:
    return "R32";
  case AVLTStateKind::R33:
    return "R33";
  case AVLTStateKind::R34:
    return "R34";
  }
}

uint64_t StateStructure::getAsUint64T() const {
  uint64_t StateAsInt = static_cast<uint64_t>(State);

  return (StateAsInt << 32) | (EvenOdd);
}

StateStructure StateStructureFromUint64T(uint64_t Int) {
  uint32_t EvenOdd = Int & 0xFFFFFFFF;
  uint64_t StateAsInt = Int >> 32;
  StateStructure Structure;

  Structure.EvenOdd = EvenOdd;
  Structure.State = static_cast<RBTStateKind>(StateAsInt);

  return Structure;
}

uint64_t AVLTStateStructure::getAsUint64T() const {
  uint64_t StateAsInt = static_cast<uint64_t>(State);

  return (StateAsInt << 32) | (EvenOdd);
}

AVLTStateStructure AVLTStateStructureFromUint64T(uint64_t Int) {
  uint32_t EvenOdd = Int & 0xFFFFFFFF;
  uint64_t StateAsInt = Int >> 32;
  AVLTStateStructure Structure;

  Structure.EvenOdd = EvenOdd;
  Structure.State = static_cast<AVLTStateKind>(StateAsInt);

  return Structure;
}
