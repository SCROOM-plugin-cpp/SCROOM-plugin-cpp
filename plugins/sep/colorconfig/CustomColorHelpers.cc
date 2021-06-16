//
// Created by jelle on 09-06-21.
//

#include "CustomColorHelpers.hh"

uint8_t CustomColorHelpers::toUint8(int32_t value) {
  value = value > 255 ? 255 : value;
  return value < 0 ? 0 : value;
}
