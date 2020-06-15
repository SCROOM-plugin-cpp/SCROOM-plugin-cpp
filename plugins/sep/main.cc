#include <stdio.h>

#include <scroom/scroomplugin.hh>
#include <scroom/unused.hh>

#include "sep.hh"

G_MODULE_EXPORT const gchar* g_module_check_init(GModule*) {
  return nullptr; // success
}

G_MODULE_EXPORT void g_module_unload(GModule*) { }

G_MODULE_EXPORT PluginInformationInterface::Ptr getPluginInformation() {
  return Sep::create();
}
