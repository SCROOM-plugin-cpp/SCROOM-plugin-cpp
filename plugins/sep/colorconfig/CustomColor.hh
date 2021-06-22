//
// Created by jelle on 02/06/2021.
//

#pragma once
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <cmath>

class CustomColor {
public:
  using Ptr = boost::shared_ptr<CustomColor>;

  std::string name;
  std::vector<std::string> aliases;
  double cMultiplier;
  double mMultiplier;
  double yMultiplier;
  double kMultiplier;

  /*
   * Scaled values are the same as the multipliers, but ints multiplied by 512
   */
  int16_t cScaled;
  int16_t mScaled;
  int16_t yScaled;
  int16_t kScaled;

  CustomColor(std::string colorName, double c, double m, double y, double k) {
    name = std::move(colorName);
    cMultiplier = c;
    mMultiplier = m;
    yMultiplier = y;
    kMultiplier = k;
    cScaled = static_cast<int16_t>(round(c * 256));
    mScaled = static_cast<int16_t>(round(m * 256));
    yScaled = static_cast<int16_t>(round(y * 256));
    kScaled = static_cast<int16_t>(round(k * 256));
  }
};
