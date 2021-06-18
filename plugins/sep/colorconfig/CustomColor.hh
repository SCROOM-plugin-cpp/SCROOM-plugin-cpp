//
// Created by jelle on 02/06/2021.
//

#pragma once
#include <string>
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
    name = colorName;
    cMultiplier = c;
    mMultiplier = m;
    yMultiplier = y;
    kMultiplier = k;
  }

  std::string getName() { return name; }

  std::vector<std::string> getAliases() { return aliases; }

  void setAliases(std::vector<std::string> newAliasses) {
    aliases = newAliasses;
  }
};


