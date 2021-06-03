//
// Created by developer on 01-06-21.
//

#include <stdio.h>

#include <scroom/scroomplugin.hh>
#include <scroom/unused.hh>

#include "CustomColorConfig.hh"



PluginInformationInterface::Ptr getPluginInformation() { return ColorConfig::create(); }