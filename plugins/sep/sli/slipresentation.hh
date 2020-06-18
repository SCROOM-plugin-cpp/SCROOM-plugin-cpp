#pragma once

#include <fstream>

#include <scroom/presentationinterface.hh>
#include <scroom/scroominterface.hh>
#include <scroom/transformpresentation.hh>
#include <scroom/pipetteviewinterface.hh>

#include "slilayer.hh"
#include "slicontrolpanel.hh"
#include "sli-helpers.hh"
#include "slisource.hh"
#include "../varnish/varnish.hh"


class SliPresentation : public PresentationBase,
                        public virtual Scroom::Utils::Base,
                        public SliPresentationInterface,
                        public PipetteViewInterface
{
public:
  typedef boost::shared_ptr<SliPresentation> Ptr;
  typedef boost::weak_ptr<SliPresentation> WeakPtr;

  /** Contains information on the aspect ratio and is used to scale the presentation accordingly */
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

  /** Callback to the own triggerRedraw function. Is passed to other classes to allow them to redraw. */
  boost::function<void()> triggerRedrawFunc;

  /** Weak pointer to this. Needed for passing a reference to SliControlPanel */
  static SliPresentationInterface::WeakPtr weakPtrToThis;

  /** The number of pixels per ResolutionUnit in the ImageWidth direction */
  float Xresolution = -1;

  /** The number of pixels per ResolutionUnit in the ImageLength direction */
  float Yresolution = -1;

  /** (Optional) the object used to draw the varnish overlay */
  Varnish::Ptr varnish;

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
  virtual bool load(const std::string& fileName);

  /** 
   * Parse the SLI file and trigger creation of the layers
   * @param fileName the absolute path of the .sli file to be parsed
   */
  virtual bool parseSli(const std::string &fileName);

  /** Get a reference to the list of all layers in SliSource */
  std::vector<SliLayer::Ptr>& getLayers() {return source->layers;};

  ////////////////////////////////////////////////////////////////////////
  // SliPresentationInterface
  ////////////////////////////////////////////////////////////////////////

  /** 
   *  Erase (delete reference) the RGB cache of the SliSource except for the bottom layer
   *  for which the relevant bytes are simply turned to 0s.
   */
  virtual void wipeCache();

  /** Causes the SliPresentation to redraw the current presentation */
  virtual void triggerRedraw();
  
  /** Get a copy of the bitmap encoding the visibility of layers from SliSource */
  virtual boost::dynamic_bitset<> getVisible();

  /** Set the bits of the newly toggled bits in the toggled bitmap of SliSource */
  virtual void setToggled(boost::dynamic_bitset<> bitmap);

  ////////////////////////////////////////////////////////////////////////
  // PresentationInterface
  ////////////////////////////////////////////////////////////////////////

  virtual Scroom::Utils::Rectangle<double> getRect();
  virtual void redraw(ViewInterface::Ptr const& vi, cairo_t* cr, Scroom::Utils::Rectangle<double> presentationArea, int zoom);
  virtual bool getProperty(const std::string& name, std::string& value);
  virtual bool isPropertyDefined(const std::string& name);
  virtual std::string getTitle();

  ////////////////////////////////////////////////////////////////////////
  // PresentationBase
  ////////////////////////////////////////////////////////////////////////

  virtual void viewAdded(ViewInterface::WeakPtr vi);
  virtual void viewRemoved(ViewInterface::WeakPtr vi);
  virtual std::set<ViewInterface::WeakPtr> getViews();
  
  ////////////////////////////////////////////////////////////////////////
  // PipetteViewInterface
  ////////////////////////////////////////////////////////////////////////

  virtual PipetteLayerOperations::PipetteColor getPixelAverages(Scroom::Utils::Rectangle<int> area);
};
