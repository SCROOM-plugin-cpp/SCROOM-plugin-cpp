//
// Created by jelle on 02/06/2021.
//

#pragma once
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

class CustomColor {
public:
  using Ptr = boost::shared_ptr<CustomColor>;

  std::string name;
  std::vector<std::string> aliases;
  double cMultiplier;
  double mMultiplier;
  double yMultiplier;
  double kMultiplier;
  CustomColor(std::string colorName, double c, double m, double y, double k) {
    name = std::move(colorName);
    cMultiplier = c;
    mMultiplier = m;
    yMultiplier = y;
    kMultiplier = k;
  }
};


