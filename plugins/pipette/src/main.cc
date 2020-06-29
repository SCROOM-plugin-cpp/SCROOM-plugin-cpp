#include <stdio.h>

#include <scroom/scroomplugin.hh>

#include "pipette.hh"

G_MODULE_EXPORT const gchar *g_module_check_init(GModule *) {
  printf("Pipette plugin check_init function\n");
  return nullptr; // success
}

G_MODULE_EXPORT void g_module_unload(GModule *) {
  printf("Pipette plugin unload function\n");
}

G_MODULE_EXPORT PluginInformationInterface::Ptr getPluginInformation() {
  return Pipette::create();
}
