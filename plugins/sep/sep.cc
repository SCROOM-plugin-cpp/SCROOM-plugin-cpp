#include "sep.hh"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include "seppresentation.hh"
#include "sli/slipresentation.hh"

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

Sep::Sep() {
}

Sep::~Sep() {
}

Sep::Ptr Sep::create() {
    return Ptr(new Sep());
}

std::string Sep::getPluginName() {
    return "SEP and SLI";
}

std::string Sep::getPluginVersion() {
    return "0.0";
}

////////////////////////////////////////////////////////////////////////
// PluginInformationInterface
////////////////////////////////////////////////////////////////////////

void Sep::registerCapabilities(ScroomPluginInterface::Ptr host) {
    host->registerOpenInterface("SEP and SLI", shared_from_this<Sep>());
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

void Sep::open(const std::string &fileName, ScroomInterface::Ptr const &scroomInterface) {
    if (fileName.substr(fileName.find_last_of(".") + 1) == "sep") {
        printf("A SEP file will be opened\n");
        SepPresentation::Ptr presentation = SepPresentation::create(scroomInterface);
        presentation->load(fileName);
        scroomInterface->showPresentation(presentation);
    } else {
        printf("A SLI file will be opened\n");
        SliPresentation::Ptr presentation = SliPresentation::create(scroomInterface);
        presentation->load(fileName);

        TransformationData::Ptr data = presentation->transformationData;
        if (data) {
            PresentationInterface::Ptr result = TransformPresentation::create(presentation, data);
            scroomInterface->showPresentation(result);
        }
    }
}
