#include "sep.hh"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include "cpp-version.h"
#include "seppresentation.hh"
#include "sli/slipresentation.hh"

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

Sep::Sep() {}

Sep::~Sep() {}

Sep::Ptr Sep::create() { return Ptr(new Sep()); }

std::string Sep::getPluginName() { return "SEP and SLI"; }

std::string Sep::getPluginVersion() { return PACKAGE_VERSION; }

////////////////////////////////////////////////////////////////////////
// PluginInformationInterface
////////////////////////////////////////////////////////////////////////

void Sep::registerCapabilities(ScroomPluginInterface::Ptr host) {
  host->registerOpenInterface(getPluginName(), shared_from_this<Sep>());
}

////////////////////////////////////////////////////////////////////////
// OpenPresentationInterface
////////////////////////////////////////////////////////////////////////

std::list<GtkFileFilter *> Sep::getFilters() {
  std::list<GtkFileFilter *> result;

  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "SEP/SLI files");
  gtk_file_filter_add_pattern(filter, "*.sep");
  gtk_file_filter_add_pattern(filter, "*.sli");
  result.push_back(filter);

  return result;
}

void Sep::open(const std::string &fileName,
               ScroomInterface::Ptr const &scroomInterface) {
  if (boost::filesystem::path(fileName).extension() == ".sep") {
    printf("A SEP file will be opened\n");
    SepPresentation::Ptr presentation = SepPresentation::create();
    presentation->load(fileName);

    TransformationData::Ptr data = presentation->getTransform();
    if (data) {
      PresentationInterface::Ptr result =
          TransformPresentation::create(presentation, data);
      scroomInterface->showPresentation(result);
    }
  } else {
    SliPresentation::Ptr presentation =
        SliPresentation::create(scroomInterface);

    if (presentation->load(fileName)) {
      PresentationInterface::Ptr result = TransformPresentation::create(
          presentation, presentation->transformationData);
      scroomInterface->showPresentation(result);
    }
  }
}
