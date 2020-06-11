#pragma once

#include <fstream>

#include <scroom/presentationinterface.hh>
#include <scroom/scroominterface.hh>
#include <scroom/threadpool.hh>
#include <scroom/transformpresentation.hh>

#include "slilayer.hh"
#include "slicontrolpanel.hh"
#include "sli-helpers.hh"


class SliPresentation : public PresentationBase,
                        public virtual Scroom::Utils::Base,
                        public SliPresentationInterface
{
public:
  typedef boost::shared_ptr<SliPresentation> Ptr;
  typedef boost::weak_ptr<SliPresentation> WeakPtr;

  /** Contains aspect ration information for proper scaling */
  TransformationData::Ptr transformationData;

  /** The SliLayers that are part of the presentation */
  std::vector<SliLayer::Ptr> layers;

private:
  std::map<std::string, std::string> properties;
  std::set<ViewInterface::WeakPtr> views;

  /** Reference to the ScroomInterface, needed for showing presentation */
  ScroomInterface::Ptr scroomInterface;

  /** Reference to the associated SliControlPanel */
  SliControlPanel::Ptr controlPanel;

  /** Weak pointer to this. Needed for passing a reference to SliControlPanel */
  static SliPresentationInterface::WeakPtr weakPtrToThis;

  /** Width of all layers combined */
  int total_width = 0;

  /** Height of all layers combined */
  int total_height = 0;

  /** Area of all layers and offsets combined */
  int total_area;

  /** Area of all layers and offsets combined in bytes */
  int total_area_bytes;

  /** The number of pixels per ResolutionUnit in the ImageWidth direction */
  float Xresolution;

  /** The number of pixels per ResolutionUnit in the ImageLength direction */
  float Yresolution;

  /** Index of the last toggled layer */
  int lastToggledLayer = -1;

  /** 
   * Contains the cairo surfaces for the different zoom levels. 
   * The zoom level is the key, the pointer to the surface the value.
   */
  std::map<int, SurfaceWrapper::Ptr> rgbCache;

  /** The thread queue into which caching jobs are enqueued */
  ThreadPool::Queue::Ptr threadQueue;

  /** Must be acquired by a thread before modifying the cache */
  boost::mutex cachingMtx;

private:
  /** Constructor */
  SliPresentation(ScroomInterface::Ptr scroomInterface);

  /**
   * Computes the RGB bitmap without any reductions from the CMYK layer data
   */
  virtual void computeRgb();

  /**
   * Compute the reduced RGB bitmap for the given zoom. When reducing from zoom level x to x-1, 
   * a 2x2 pixel square of level x is converted to a single pixel of level x-1.
   */
  virtual void reduceRgb(int zoom);

  /** Given the zoom level, computes all RGB bitmaps that are required for displaying this zoom level
   *  May be very computationally expensive and should therefore be executed in a separate thread.
   *  To avoid race conditions, it disables the multilayer UI during caching operations.
   */
  virtual void fillCache(int zoom);

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

  /** Compute the overall width and height of the SLi file (over all layers) */
  virtual void computeHeightWidth();

  //TransformationData::Ptr getTransformationData() const;

  ////////////////////////////////////////////////////////////////////////
  // SliPresentationInterface
  ////////////////////////////////////////////////////////////////////////

  /** Wipe the zoom level RGB cache of the presentation. Needed when layers are enabled or disabled. */
  virtual void wipeCache();

  /** Causes the complete canvas to be redrawn */
  virtual void triggerRedraw();

  /** Sets the index of the last toggled layer */
  virtual void setLastToggled(int index);

  /** Getter for the layers that the presentation consists of */
  std::vector<SliLayer::Ptr>& getLayers() {return layers;};

  /** Clear the last modified area of the bottom surface */
  virtual void clearBottomSurface();

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
