#pragma once

#include <scroom/scroominterface.hh>
#include <scroom/threadpool.hh>

#include <boost/dynamic_bitset.hpp>

#include "sli-helpers.hh"

class SliSource: public virtual Scroom::Utils::Base
{
public:
  typedef boost::shared_ptr<SliSource> Ptr;
  typedef boost::weak_ptr<SliSource> WeakPtr;

  /** The SliLayers that are part of the presentation */
  std::vector<SliLayer::Ptr> layers;

  /** Width of all layers combined */
  int total_width = 0;

  /** Height of all layers combined */
  int total_height = 0;

  /** Whether any of the layers has an xoffset */
  bool hasXoffsets;

  /** Bitmask representing the indexes of the currently visible layers (little-endian) */
  boost::dynamic_bitset<> visible {0};

  /** Bitmask representing the indexes of the layers that need to be toggled (little-endian) */
  boost::dynamic_bitset<> toggled {0};

  /** Callback to enable interaction with widgets in the sidebar */
  boost::function<void()> enableInteractions;

  /** Callback to disable interaction with widgets in the sidebar */
  boost::function<void()> disableInteractions;

private:
  /** 
   * Contains the cached bitmaps for the different zoom levels. 
   * The zoom level is the key, the pointer to the bitmap the value.
   */
  std::map<int, SurfaceWrapper::Ptr> rgbCache;

  /** The thread queue into which caching jobs are enqueued */
  ThreadPool::Queue::Ptr threadQueue;

  /** Must be acquired by a thread before writing to the cached surfaces */
  boost::mutex mtx;

  /** Callback to trigger a redraw of the presentation */
  boost::function<void()> triggerRedraw;

private:
  /** Constructor */
  SliSource(boost::function<void()>& triggerRedrawFunc);

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
   * @param zoom the zoom level for which to fill the cache
   */
  virtual void fillCache(int zoom);
  
  /** Clear the last modified area of the bottom surface */
  virtual void clearBottomSurface();

public:
  /** Destructor */
  virtual ~SliSource();

  /** 
   * Constructor 
   * @param triggerRedrawFunc a callback that allows for triggering a redraw of the presentation
   */
  static Ptr create(boost::function<void ()>& triggerRedrawFunc);

  /** Compute the overall width and height of the SLi file (over all layers) */
  virtual void computeHeightWidth();

  /** Checks if any of the layers has an xoffset */
  virtual void checkXoffsets();

  /**
   * Get the SurfaceWrapper for the surface that is needed to display the zoom level. 
   * If it is not cached yet, enqueue its computation
   * @param zoom the zoom level for which to return the SurfaceWrapper
   * @return the SurfaceWrapper needed for displaying the zoom level or nullptr if it is not cached yet
   */
  virtual SurfaceWrapper::Ptr getSurface(int zoom);

  virtual bool addLayer(std::string imagePath, std::string filename, int xOffset, int yOffset);

  /** Wipe the zoom level RGB cache of the presentation. Needed when layers are enabled or disabled. */
  virtual void wipeCache();

  /* Draw the CMYK data on the surface (more efficient) */
  virtual void drawCmyk(uint8_t *surfacePointer, uint8_t *bitmap, int bitmapStart, int bitmapOffset);

  /* Draw the CMYK data on the surface */
  virtual void drawCmykXoffset(uint8_t *surfacePointer, uint8_t *bitmap, int bitmapStart, int bitmapOffset, Scroom::Utils::Rectangle<int> layerRect, Scroom::Utils::Rectangle<int> intersectRect, int layerBound, int stride);

  /* Convert the CMYK data on the surface to ARGB (more efficient) */
  virtual void convertCmyk(uint8_t *surfacePointer, uint32_t *targetPointer, int topLeftOffset, int bottomRightOffset);

  /* Convert the CMYK data on the surface to ARGB */
  virtual void convertCmykXoffset(uint8_t *surfacePointer, uint32_t *targetPointer, int topLeftOffset, int bottomRightOffset, Scroom::Utils::Rectangle<int> toggledRect, int imageBound, int stride);
};