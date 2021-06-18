//
// Created by developer on 01-06-21.
//

#pragma once
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>

#include "CustomColor.hh"
#include <scroom/plugininformationinterface.hh>
#include <scroom/utilities.hh>
#include <unordered_set>

namespace pt = boost::property_tree;
class ColorConfig {

private:
  ColorConfig();

private:
  std::vector<CustomColor::Ptr> colors;

public:
  static ColorConfig &getInstance() {
    static ColorConfig INSTANCE;
    return INSTANCE;
  }

  std::vector<CustomColor::Ptr> getDefinedColors();
  CustomColor::Ptr getColorByNameOrAlias(std::string name);
  void loadFile(std::string file="colours.json");

  void addNonExistentDefaultColors();


private:
    void parseColor(pt::ptree::value_type &v,
                    std::unordered_set<std::string> &seenNamesAndAliases);
};
