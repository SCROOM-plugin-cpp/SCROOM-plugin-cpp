//
// Created by jelle on 02/06/2021.
//

#ifndef SCROOMCPPPLUGINS_CUSTOMCOLOR_HH
#define SCROOMCPPPLUGINS_CUSTOMCOLOR_HH
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

class CustomColor {
public:
  using Ptr = boost::shared_ptr<CustomColor>;

  std::string name;
  std::vector<std::string> aliases;
  float cMultiplier;
  float mMultiplier;
  float yMultiplier;
  float kMultiplier;
  CustomColor(std::string colorName, float c, float m, float y, float k) {
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

#endif // SCROOMCPPPLUGINS_CUSTOMCOLOR_HH
