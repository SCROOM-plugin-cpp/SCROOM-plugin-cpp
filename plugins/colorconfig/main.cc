//
// Created by developer on 01-06-21.
//

#include <stdio.h>

#include <scroom/scroomplugin.hh>
#include <scroom/unused.hh>

#include "colorconfig.hh"

const gchar *g_module_check_init(GModule *) {
    return nullptr; // success
}

void g_module_unload(GModule *) {}

PluginInformationInterface::Ptr getPluginInformation() { return ColorConfig::create(); }