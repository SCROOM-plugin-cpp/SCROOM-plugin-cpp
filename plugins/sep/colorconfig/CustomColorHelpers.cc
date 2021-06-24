//
// Created by jelle on 09-06-21.
//
#include "CustomColorHelpers.hh"
#include <cmath>

uint8_t CustomColorHelpers::toUint8(int16_t value) {
  value = value > 255 ? 255 : value;
  return value < 0 ? 0 : value;
}

void CustomColorHelpers::calculateCMYK(CustomColor::Ptr &color, int16_t &C,
                                       int16_t &M, int16_t &Y, int16_t &K,
                                       uint8_t value) {
  C += (color->cMultiplier * value);
  M += (color->mMultiplier * value);
  Y += (color->yMultiplier * value);
  K += (color->kMultiplier * value);
}
