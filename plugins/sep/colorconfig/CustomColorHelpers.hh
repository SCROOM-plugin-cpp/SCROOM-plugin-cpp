//
// Created by jelle on 09-06-21.
//

#pragma once

#include "CustomColor.hh"
#include <cstdint>

class CustomColorHelpers {
public:
  /**
   * Convert int32 to uint8.
   * value < 0 -> 0
   * value > 255 -> 255
   * @param value int32 to convert
   * @return capped uint8 value
   */
  static uint8_t toUint8(int32_t value);

  /**
   * Execute the custom color to CMYK calculations for a single color and value
   * alters C M Y K to add or subtract the calculated CMYK values of this color
   * and intensity
   * @param color Color to use for calculation
   * @param C reference to the C value to alter
   * @param M reference to the M value to alter
   * @param Y reference to the Y value to alter
   * @param K reference to the K value to alter
   * @param value intensity of the specified color
   */
  static void calculateCMYK(CustomColor::Ptr &color, int32_t &C, int32_t &M,
                            int32_t &Y, int32_t &K, uint8_t value);
};
