#pragma once

#include <map>
#include <string>

#include <scroom/presentationinterface.hh>

#include "sepsource.hh"

////////////////////////////////////////////////////////////////
////////////// SepPresentation

class SepPresentation : public PresentationBase,
                        public virtual Scroom::Utils::Base
{

public:
    typedef boost::shared_ptr<SepPresentation> Ptr;

private:
    // This variable is only set in the constructor and never used.
    // TODO: reconsider the existence of this variable.
    ScroomInterface::Ptr scroom_interface;
    SepSource::Ptr sep_source;

    TiledBitmapInterface::Ptr tbi;
    std::string file_name;

    size_t width;
    size_t height;
    TransformationData::Ptr transform;

    std::set<ViewInterface::WeakPtr> views;

    std::map<std::string, std::string> properties;

private:
    /**
     * Constructor for a standalone SepPresentation to be passed to
     * the Scroom core.
     */
    SepPresentation(ScroomInterface::Ptr interface);

public:
    virtual ~SepPresentation();

    /**
     * Constructor to be called for a standalone SepPresentation to
     * be passed to the Scroom core
     */
    static Ptr create(ScroomInterface::Ptr interface);

    /**
     * Load the SEP file whose filename is passed as argument.
     */
    bool load(const std::string& file_name);

    /**
     * Return the transformation data for the loaded file.
     */
    TransformationData::Ptr getTransform();


    ////////////////////////////////////////////////////////////////////////
    // PresentationInterface
    ////////////////////////////////////////////////////////////////////////

    Scroom::Utils::Rectangle<double> getRect() override;
    void redraw(ViewInterface::Ptr const &vi, cairo_t *cr, Scroom::Utils::Rectangle<double> presentationArea, int zoom) override;
    bool getProperty(const std::string &name, std::string &value) override;
    bool isPropertyDefined(const std::string &name) override;
    std::string getTitle() override;

    ////////////////////////////////////////////////////////////////////////
    // PresentationBase
    ////////////////////////////////////////////////////////////////////////

    void viewAdded(ViewInterface::WeakPtr interface) override;
    void viewRemoved(ViewInterface::WeakPtr interface) override;
    std::set<ViewInterface::WeakPtr> getViews() override;
};
