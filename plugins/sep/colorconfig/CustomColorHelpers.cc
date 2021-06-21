//
// Created by jelle on 09-06-21.
//
#include "CustomColorHelpers.hh"
#include <cmath>

uint8_t CustomColorHelpers::toUint8(int32_t value) {
  value = value > 255 ? 255 : value;
  return value < 0 ? 0 : value;
}

void CustomColorHelpers::calculateCMYK(CustomColor::Ptr &color, int32_t &C,
                                       int32_t &M, int32_t &Y, int32_t &K,
                                       uint8_t value) {
  C += static_cast<int32_t>(round(color->cMultiplier * value));
  M += static_cast<int32_t>(round(color->mMultiplier * value));
  Y += static_cast<int32_t>(round(color->yMultiplier * value));
  K += static_cast<int32_t>(round(color->kMultiplier * value));
}
