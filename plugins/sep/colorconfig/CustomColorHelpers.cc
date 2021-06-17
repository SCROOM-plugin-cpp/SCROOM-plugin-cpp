//
// Created by jelle on 09-06-21.
//
#include <cmath>
#include "CustomColorHelpers.hh"

uint8_t CustomColorHelpers::toUint8(int32_t value) {
  value = value > 255 ? 255 : value;
  return value < 0 ? 0 : value;
}

void
CustomColorHelpers::calculateCMYK(CustomColor::Ptr &color, int32_t &C, int32_t &M, int32_t &Y, int32_t &K, uint8_t value) {
    C += static_cast<int32_t>(roundf(color->cMultiplier * static_cast<float>(value)));
    M += static_cast<int32_t>(roundf(color->mMultiplier * static_cast<float>(value)));
    Y += static_cast<int32_t>(roundf(color->yMultiplier * static_cast<float>(value)));
    K += static_cast<int32_t>(roundf(color->kMultiplier * static_cast<float>(value)));
}
