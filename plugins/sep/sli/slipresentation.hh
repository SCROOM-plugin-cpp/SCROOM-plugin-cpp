#pragma once

#include <fstream>

#include <scroom/presentationinterface.hh>
#include <scroom/scroominterface.hh>
#include <scroom/transformpresentation.hh>

#include "slilayer.hh"
#include "slicontrolpanel.hh"
#include "sli-helpers.hh"
#include "slisource.hh"


class SliPresentation : public PresentationBase,
                        public virtual Scroom::Utils::Base,
                        public SliPresentationInterface
{
public:
  typedef boost::shared_ptr<SliPresentation> Ptr;
  typedef boost::weak_ptr<SliPresentation> WeakPtr;

  /** Contains information on the aspect ratio and is used to scale the presentation accordingly */
  TransformationData::Ptr transformationData;

private:
  /** The properties defined for this presentation*/
  std::map<std::string, std::string> properties;

  /** The views associated with this presentation */
  std::set<ViewInterface::WeakPtr> views;

  /** Reference to the ScroomInterface, needed for showing presentation */
  ScroomInterface::Ptr scroomInterface;

  /** Reference to the associated SliControlPanel */
  SliControlPanel::Ptr controlPanel;
  
  /** Contains and manages all the bitmap data of this presentation*/
  SliSource::Ptr source;

  /** Callback to the own triggerRedraw function. Is passed to other classes to allow them to redraw. */
  boost::function<void()> triggerRedrawFunc;

  /** Weak pointer to this. Needed for passing a reference to SliControlPanel */
  static SliPresentationInterface::WeakPtr weakPtrToThis;

  /** The number of pixels per ResolutionUnit in the ImageWidth direction */
  float Xresolution;

  /** The number of pixels per ResolutionUnit in the ImageLength direction */
  float Yresolution;

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
  virtual void parseSli(const std::string &fileName);

  /** Getter for the layers that the presentation consists of */
  std::vector<SliLayer::Ptr>& getLayers() {return source->layers;};

  ////////////////////////////////////////////////////////////////////////
  // SliPresentationInterface
  ////////////////////////////////////////////////////////////////////////

  /** Wipe the zoom level RGB cache of the presentation. Needed when layers are enabled or disabled. */
  virtual void wipeCache();

  /** Causes the complete canvas to be redrawn */
  virtual void triggerRedraw();
  
  virtual boost::dynamic_bitset<> getToggled();
  virtual boost::dynamic_bitset<> getVisible();
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

  virtual void viewAdded(ViewInterface::WeakPtr viewInterface);
  virtual void viewRemoved(ViewInterface::WeakPtr vi);
  virtual std::set<ViewInterface::WeakPtr> getViews();
  
  ////////////////////////////////////////////////////////////////////////
  // PipetteViewInterface
  ////////////////////////////////////////////////////////////////////////

  //virtual void SliPresentation::getPixelAverages(Scroom::Utils::Rectangle<int> area);
};
