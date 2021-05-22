#include <stdio.h>

#include <scroom/scroomplugin.hh>
#include <scroom/unused.hh>

#include "sep.hh"

const gchar *g_module_check_init(GModule *) {
  return nullptr; // success
}

void g_module_unload(GModule *) {}

PluginInformationInterface::Ptr getPluginInformation() {
  return Sep::create();
}
