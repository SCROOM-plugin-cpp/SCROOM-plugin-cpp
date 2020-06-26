#pragma once

#include <scroom/scroominterface.hh>
#include <scroom/threadpool.hh>

#include <boost/dynamic_bitset.hpp>

#include "../sepsource.hh"
#include "sli-helpers.hh"

class SliSource : public virtual Scroom::Utils::Base {
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

  /** Whether the bitmaps of all layers have been imported from the files yet */
  bool bitmapsImported = false;

  /** Bitmask representing the indexes of the currently visible layers
   * (little-endian) */
  boost::dynamic_bitset<> visible{0};

  /** Bitmask representing the indexes of the layers that need to be toggled
   * (little-endian) */
  boost::dynamic_bitset<> toggled{0};

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

  /**
   * For each layer that represens a SEP file, this map contains the
   * corresponding SepSource between reading the metadata of the layer and
   * reading the bitmap. Afterwards, it's cleared out to save memory.
   */
  std::map<SliLayer::Ptr, SepSource::Ptr> sepSources;

private:
  /** Constructor */
  SliSource(boost::function<void()> &triggerRedrawFunc);

  /**
   * Computes the complete RGB bitmap from the CMYK bitmap and caches the result
   */
  virtual void computeRgb();

  /**
   * Reduces the RGB bitmap and caches the result.
   * The bitmap is divided into roughly 24+ segments whose height is a multiple of 2.
   * This creates a one-to-one mapping bewteen each pixel of zoom level @param zoom
   * and a 2x2 square of pixels of zoom level @param zoom+1.
   * @param multithreading indicates whether more than one thread will "possibly" be used for the reduction.
   * This choice also depends on the number of toggled segments: if only a handful are toggled,
   * the additional overhead of threads is not worth it.
   */
  virtual void reduceRgb(int zoom, bool multithreading);

  /**
   * Reduces all the segments set in @param toggledSegments for zoom level @param zoom.
   * @param baseSegHeight is the height of a segment in the base surface (zoom level 0).
   * @param targetSurface is the surface on top of which the reduced segments will be copied.
   */
  virtual void reduceSegments(SurfaceWrapper::Ptr targetSurface, boost::dynamic_bitset<> toggledSegments,
                            int baseSegHeight, int zoom);

  /**
   * Checks if the bitmap required for displaying the zoom level is present. If
   * not, it is computed. Is potentially very computationally expensive, hence
   * run outside of the UI thread.
   */
  virtual void fillCache();

  /** Clear the last modified area of the bottom surface */
  virtual void clearBottomSurface();

  /**
   * Draw the CMYK data onto the surface.
   * @param surfacePointer is a pointer to the byte of the surface where the
   * drawing will start.
   * @param bitmap holds a pointer to the CMYK bitmap to draw.
   * @param bitmapStart is the index into the start of the bitmap area to draw.
   * @param bitmapOffset is the offset from bitmapStart of the bitmap area to
   * draw.
   */
  virtual void drawCmyk(uint8_t *surfacePointer, uint8_t *bitmap,
                        int bitmapStart, int bitmapOffset);

  /**
   * Draw the CMYK data onto the surface. It is similar to drawCmyk but it also
   * supports SLI files with xoffsets. However, it is noticeably slower.
   * @param surfacePointer is a pointer to the byte of the surface where the
   * drawing will start.
   * @param bitmap holds a pointer to the CMYK bitmap to draw.
   * @param bitmapStart is the index into the start of the bitmap area to draw.
   * @param bitmapOffset is the offset from bitmapStart of the bitmap area to
   * draw.
   * @param layerRect represents the current layer being drawn.
   * @param intersectRect represents the area of the current layer that
   * intersects the canvas.
   * @param layerBound is the right bound of area to draw.
   * @param stride is the stride of the entire SLI image.
   */
  virtual void drawCmykXoffset(uint8_t *surfacePointer, uint8_t *bitmap,
                               int bitmapStart, int bitmapOffset,
                               Scroom::Utils::Rectangle<int> layerRect,
                               Scroom::Utils::Rectangle<int> intersectRect,
                               int layerBound, int stride);

  /**
   * Converts the a CMYK surface to an RGB surface.
   * @param surfacePointer is a pointer to the first byte of the surface.
   * @param targetPointer is a pointer to the first pixel of the surface.
   * @param topLeftOffset is the offset from coordinate (0,0) of the first byte
   * of the surface.
   * @param bottomRightOffset is the offset from coordinate (0,0) of the last
   * byte of the surface.
   */
  virtual void convertCmyk(uint8_t *surfacePointer, uint32_t *targetPointer,
                           int topLeftOffset, int bottomRightOffset);

  /**
   * Converts the a CMYK surface to an RGB surface. It is similar to convertCmyk
   * but it also supports SLI files with xoffsets. However, it is noticeably
   * slower.
   * @param surfacePointer is a pointer to the first byte of the surface.
   * @param targetPointer is a pointer to the first pixel of the surface.
   * @param topLeftOffset is the offset from coordinate (0,0) of the first byte
   * of the surface.
   * @param bottomRightOffset is the offset from coordinate (0,0) of the last
   * byte of the surface.
   * @param toggledWidth represents the width of the area of the surface that
   * needs to be converted.
   * @param toggledBound represents the right bound of the area to convert.
   * @param stride is the stride of the entire SLI image.
   */
  virtual void convertCmykXoffset(uint8_t *surfacePointer,
                                  uint32_t *targetPointer, int topLeftOffset,
                                  int bottomRightOffset, int toggledWidth,
                                  int toggledBound, int stride);

  /**
   * For each SliLayer in layers, import the bitmap data from the file into the
   * SliLayer. Computationally intensive, therefore done outside of UI thread.
   */
  virtual void importBitmaps();

public:
  /** Destructor */
  virtual ~SliSource();

  /**
   * Constructor
   * @param triggerRedrawFunc a callback that allows for triggering a redraw of
   * the presentation
   */
  static Ptr create(boost::function<void()> &triggerRedrawFunc);

  /** Compute the overall width and height of the SLi file (over all layers) */
  virtual void computeHeightWidth();

  /**
   * Check if any of the layers has an xoffset.
   * Drawing Sli layers with xoffsets requires many checks to know when to jump
   * to the next line. These degrade performance so we try to avoid them if
   * possible.
   */
  virtual void checkXoffsets();

  /**
   * Get the SurfaceWrapper for the surface that is needed to display the zoom
   * level. If it is not cached yet, enqueue its computation
   * @param zoom the zoom level for which to return the SurfaceWrapper
   * @return the SurfaceWrapper needed for displaying the zoom level or nullptr
   * if it is not cached yet
   */
  virtual SurfaceWrapper::Ptr getSurface(int zoom);

  /**
   * Create a new SliLayer and add it to the list of layers.
   * @param imagePath is the absolute path to the tif/sep file.
   * @param filename is the name of the file, including its extension.
   * @param xOffset the xoffset of the layer defined in the SLI file.
   * @param yOffset the yoffset of the layer defined in the SLI file.
   */
  virtual bool addLayer(std::string imagePath, std::string filename,
                        int xOffset, int yOffset);

  /**
   * Query the execution of importBitmaps() in a separate thread.
   */
  virtual void queryImportBitmaps();

  /**
   *  Erase the RGB cache of the SliSource except for the bottom layer
   *  for which the relevant bytes are simply turned to 0s.
   */
  virtual void wipeCache();
};