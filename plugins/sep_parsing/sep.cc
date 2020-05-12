#include "sepparser.hh"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "scroom/transformpresentation.hh"

#include "sepparserpresentation.hh"

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

SepParser::SepParser(){
}

SepParser::~SepParser(){
}

Sep::Ptr Sep::create(){
  return Ptr(new Sep());
}

std::string SepParser::getPluginName(){
  return "SEP";
}

std::string SepParser::getPluginVersion(){
  return "0.0";
}

////////////////////////////////////////////////////////////////////////
// PluginInformationInterface
////////////////////////////////////////////////////////////////////////

void SepParser::registerCapabilities(ScroomPluginInterface::Ptr host){
	host->registerOpenPresentationInterface("SEP viewer", shared_from_this<Sep>());
}

////////////////////////////////////////////////////////////////////////
// OpenPresentationInterface
////////////////////////////////////////////////////////////////////////

std::list<GtkFileFilter*> Sep::getFilters()
{
  std::list<GtkFileFilter*> result;

  GtkFileFilter* filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "SEP files");
  gtk_file_filter_add_mime_type(filter, "image/sep");
  result.push_back(filter);

  return result;
}

PresentationInterface::Ptr Sep::open(const std::string& fileName)
{
  SepPresentationWrapper::Ptr wrapper = SepPresentationWrapper::create();
  if(!wrapper->load(fileName))
  {
    wrapper.reset();
  }
  PresentationInterface::Ptr result = wrapper;
  if(result)
  {
    TransformationData::Ptr data = wrapper->getTransformationData();
    if(data)
    {
      result = TransformPresentation::create(result, data);
    }
  }
  return result;
}
