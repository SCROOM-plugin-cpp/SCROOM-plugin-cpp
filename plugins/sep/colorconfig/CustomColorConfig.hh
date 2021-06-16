//
// Created by developer on 01-06-21.
//

#pragma once
#include <iostream>
#include <jsoncpp/json/value.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>

#include <scroom/plugininformationinterface.hh>
#include <scroom/utilities.hh>
#include "CustomColor.hh"

namespace pt = boost::property_tree;
class ColorConfig{

private:
  ColorConfig();

private:
  std::vector<CustomColor>* colors;


public:
  static ColorConfig& getInstance(){
      static ColorConfig INSTANCE;
      return INSTANCE;
  }

  std::vector<CustomColor>* getDefinedColors();
  CustomColor* getColorByNameOrAlias(std::string name);
  void loadFile();

  void addNonExistentDefaultColors();
};

