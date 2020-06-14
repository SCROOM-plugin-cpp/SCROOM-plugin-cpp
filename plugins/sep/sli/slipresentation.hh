#pragma once

#include <fstream>

#include <scroom/presentationinterface.hh>
#include <scroom/scroominterface.hh>
#include <scroom/threadpool.hh>
#include <scroom/transformpresentation.hh>
#include <boost/dynamic_bitset.hpp>

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
  TransformationData::Ptr transformationData;

private:
  std::map<std::string, std::string> properties;
  std::set<ViewInterface::WeakPtr> views;

  /** The SliLayers that are part of the presentation */
  std::vector<SliLayer::Ptr> layers;

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

  /** The number of pixels per ResolutionUnit in the ImageWidth direction */
  float Xresolution;

  /** The number of pixels per ResolutionUnit in the ImageLength direction */
  float Yresolution;

  /** (Optional) A file that specifies the varnish mask */
  std::string varnishFile;

  /** Bitmap representing the indexes of the currently visible layers (little-endian) */
  boost::dynamic_bitset<> visible {0};

  /** Bitmask representing the indexes of the layers that need to be toggled (little-endian) */
  boost::dynamic_bitset<> toggled {0};

  /** 
   * Contains the cached bitmaps for the different zoom levels. 
   * The zoom level is the key, the pointer to the bitmap the value.
   */
  std::map<int, SurfaceWrapper::Ptr> rgbCache;

  /** The thread queue into which caching jobs are enqueued */
  ThreadPool::Queue::Ptr threadQueue;

  /** Must be acquired by a thread before writing to the cached surfaces */
  boost::mutex mtx;

private:
  /** Constructor */
  SliPresentation(ScroomInterface::Ptr scroomInterface);

  /**
   * Computes the complete RGB bitmap from the CMYK bitmap and caches the result
   */
  virtual void computeRgb();

  /**
   * Reduces the RGB bitmap and caches the result. 
   * 2x2 pixels of the zoom+1 bitmap are combined into one pixel of the zoom bitmap
   */
  virtual void reduceRgb(int zoom);

  /** 
   * Checks if the bitmap required for displaying the zoom level is present. If not, it is computed.
   * Is potentially very computationally expensive, hence run outside of the UI thread.
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

  /** Getter for the layers that the presentation consists of */
  std::vector<SliLayer::Ptr>& getLayers() {return layers;};

  /** Clear the last modified area of the bottom surface */
  virtual void clearBottomSurface();

  TransformationData::Ptr getTransformationData() const;

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
  virtual void setVisible(boost::dynamic_bitset<> bitmap);

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
