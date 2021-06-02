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
#include "custom-colors.hh"

namespace pt = boost::property_tree;
class ColorConfig
  : public PluginInformationInterface,
    virtual public Scroom::Utils::Base
{
public:
  using Ptr = boost::shared_ptr<ColorConfig>;

private:
  ColorConfig();

public:
    /** Get a reference to the instance */
    static ColorConfig& getInstance();

public:
  static Ptr create();


public:
  std::string getPluginName() override;
  std::string getPluginVersion() override;
  void        registerCapabilities(ScroomPluginInterface::Ptr host) override;
  void loadFile();
  std::vector<Colour> colours = {};
};
