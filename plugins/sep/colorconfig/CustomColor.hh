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

  // Use floats, as this is faster than using doubles, and we do not need the
  // extra precision
  float cMultiplier;
  float mMultiplier;
  float yMultiplier;
  float kMultiplier;

  CustomColor(std::string colorName, float c, float m, float y, float k) {
    name = std::move(colorName);
    cMultiplier = c;
    mMultiplier = m;
    yMultiplier = y;
    kMultiplier = k;
  }
};
