//
// Created by jelle on 09-06-21.
//

#include "CustomColorHelpers.hh"

uint8_t CustomColorHelpers::toUint8(int32_t value) {
    if (value > 255) return 255;
    if (value < 0) return 0;
    return value;
}
