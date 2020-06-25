#pragma once

#include <fstream>

#include <scroom/pipetteviewinterface.hh>
#include <scroom/presentationinterface.hh>
#include <scroom/scroominterface.hh>
#include <scroom/transformpresentation.hh>

#include "../varnish/varnish.hh"
#include "sli-helpers.hh"
#include "slicontrolpanel.hh"
#include "slilayer.hh"
#include "slisource.hh"

class SliPresentation : public PresentationBase,
                        public virtual Scroom::Utils::Base,
                        public SliPresentationInterface,
                        public PipetteViewInterface {
public:
  typedef boost::shared_ptr<SliPresentation> Ptr;
  typedef boost::weak_ptr<SliPresentation> WeakPtr;

  /** Contains information on the aspect ratio and is used to scale the
   * presentation accordingly */
  TransformationData::Ptr transformationData;

private:
  /** The properties defined for this presentation */
  std::map<std::string, std::string> properties;

  /** The views associated with this presentation */
  std::set<ViewInterface::WeakPtr> views;

  /** Reference to the ScroomInterface, needed for showing presentation */
  ScroomInterface::Ptr scroomInterface;

  /** Reference to the associated SliControlPanel */
  SliControlPanel::Ptr controlPanel;

  /** Contains and manages all the bitmap data of this presentation */
  SliSource::Ptr source;

  /** Callback to the own triggerRedraw function. Is passed to other classes to
   * allow them to redraw. */
  boost::function<void()> triggerRedrawFunc;

  /** Weak pointer to this. Needed for passing a reference to SliControlPanel */
  static SliPresentationInterface::WeakPtr weakPtrToThis;

  /** The number of pixels per ResolutionUnit in the ImageWidth direction */
  float Xresolution = -1;

  /** The number of pixels per ResolutionUnit in the ImageLength direction */
  float Yresolution = -1;

  /** (Optional) the object used to draw the varnish overlay */
  Varnish::Ptr varnish;

  /** The absolute path to the SLI file */
  std::string filepath;

private:
  /** Constructor */
  SliPresentation(ScroomInterface::Ptr scroomInterface);

public:
  /** Destructor */
  virtual ~SliPresentation();

  /** Constructor */
  static Ptr create(ScroomInterface::Ptr scroomInterface);

  /**
   * Load the SLI file and instruct the Scroom core to display it
   * @param fileName the absolute path of the .sli file to be opened
   */
  bool load(const std::string &fileName);

  /**
   * Parse the SLI file and trigger creation of the layers
   * @param fileName the absolute path of the .sli file to be parsed
   */
  bool parseSli(const std::string &fileName);

  /** Get a reference to the list of all layers in SliSource */
  std::vector<SliLayer::Ptr> &getLayers() { return source->layers; };

  ////////////////////////////////////////////////////////////////////////
  // SliPresentationInterface
  ////////////////////////////////////////////////////////////////////////

  /**
   *  Erase (delete reference) the RGB cache of the SliSource except for the
   * bottom layer for which the relevant bytes are simply turned to 0s.
   */
  void wipeCache() override;

  /** Causes the SliPresentation to redraw the current presentation */
  void triggerRedraw() override;

  /** Get a copy of the bitmap encoding the visibility of layers from SliSource
   */
  boost::dynamic_bitset<> getVisible() override;

  /** Set the bits of the newly toggled bits in the toggled bitmap of SliSource
   */
  void setToggled(boost::dynamic_bitset<> bitmap) override;

  ////////////////////////////////////////////////////////////////////////
  // PresentationInterface
  ////////////////////////////////////////////////////////////////////////

  Scroom::Utils::Rectangle<double> getRect() override;
  void redraw(ViewInterface::Ptr const &vi, cairo_t *cr,
              Scroom::Utils::Rectangle<double> presentationArea,
              int zoom) override;
  bool getProperty(const std::string &name, std::string &value) override;
  bool isPropertyDefined(const std::string &name) override;
  std::string getTitle() override;

  ////////////////////////////////////////////////////////////////////////
  // PresentationBase
  ////////////////////////////////////////////////////////////////////////

  void viewAdded(ViewInterface::WeakPtr vi) override;
  void viewRemoved(ViewInterface::WeakPtr vi) override;
  std::set<ViewInterface::WeakPtr> getViews() override;

  ////////////////////////////////////////////////////////////////////////
  // PipetteViewInterface
  ////////////////////////////////////////////////////////////////////////

  PipetteLayerOperations::PipetteColor
  getPixelAverages(Scroom::Utils::Rectangle<int> area) override;
};
