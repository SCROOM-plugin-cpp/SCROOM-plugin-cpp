#include "pipette.hh"

#include <gdk/gdk.h>

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

Pipette::Pipette()
{
}
Pipette::~Pipette()
{
}

Pipette::Ptr Pipette::create()
{
  return Ptr(new Pipette());
}

std::string Pipette::getPluginName()
{
  return "Pipette";
}

std::string Pipette::getPluginVersion()
{
  return "0.0";
}

void Pipette::registerCapabilities(ScroomPluginInterface::Ptr host)
{
  //dunno what we can do yet, need some sort of UI mouse interaction hook
}
