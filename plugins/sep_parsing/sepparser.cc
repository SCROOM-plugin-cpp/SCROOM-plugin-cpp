#include "sepparser.hh"

#include <gdk/gdk.h>

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

SepParser::SepParser(){
}

Parse::~SepParser(){
}

SepParser::Ptr SepParser::create()
{
  return Ptr(new SepParser());
}

std::string SepParser::getPluginName()
{
  return "SEP parser";
}

std::string SepParser::getPluginVersion()
{
  return "0.0";
}

void SepParser::registerCapabilities(ScroomPluginInterface::Ptr host)
{
  //host->registerNewPresentationInterface("SEP Parser", shared_from_this<SepParser>());
}

PresentationInterface::Ptr Example::createNew()
{
  return PresentationInterface::Ptr(new SepParserPresentation());
}

