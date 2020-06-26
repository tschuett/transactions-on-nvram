#include "tree.h"

#include <string>

std::string Color2String(Color C) {
  switch(C) {
  case Color::Red:
    return "Red";
  case Color::Black:
    return "Black";
  case Color::Unknown:
    return "Unknown";
  }
}

std::string Direction2String(Direction Dir) {
  switch(Dir) {
  case Direction::Left:
    return "Left";
  case Direction::Right:
    return "Right";
  case Direction::Unknown:
    return "Unknown";
  }
}
