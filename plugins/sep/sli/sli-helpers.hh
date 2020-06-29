#pragma once

#include <boost/dynamic_bitset.hpp>
#include <cairo.h>
#include <scroom/point.hh>
#include <scroom/rectangle.hh>
#include <scroom/utilities.hh>

#include "slilayer.hh"

class SurfaceWrapper : public virtual Scroom::Utils::Base {
public:
  typedef boost::shared_ptr<SurfaceWrapper> Ptr;

  /** The cairo surface wrapped by this class */
  cairo_surface_t *surface;

  /** Whether the surface contents have been cleared
   * Mostly used for the bottom layer surface
   */
  bool clear;

private:
  /** Used to destroy SurfaceWrapper pointers with no surface */
  bool empty;
  SurfaceWrapper();
  SurfaceWrapper(int width, int height, cairo_format_t format);

public:
  /** Constructors */
  static Ptr create();
  static Ptr create(int width, int height, cairo_format_t format);

  /** Get the height of the wrapped surface */
  virtual int getHeight();

  /** Get the width of the wrapped surface */
  virtual int getWidth();

  /** Get the stride of the wrapped surface */
  virtual int getStride();

  /** Get the bitmap of the wrapped surface */
  virtual uint8_t *getBitmap();

  /** Fill the entire surface with 0s */
  virtual void clearSurface();

  /** Fill a rectangle of the surface with 0s */
  virtual void clearSurface(Scroom::Utils::Rectangle<int> rect);

  /** Return the Rectangle representation of the surface (in pixels) */
  virtual Scroom::Utils::Rectangle<int> toRectangle();

  /** Destructor */
  virtual ~SurfaceWrapper();
};

/** Compute area in pixels of the given rectangle */
int getArea(Scroom::Utils::Rectangle<int> rect);

/** Stretch the rectangle @param bpp (Bytes Per Pixel, 4 for this plugin) times
 * horizontally */
Scroom::Utils::Rectangle<int>
toBytesRectangle(Scroom::Utils::Rectangle<int> rect, int bpp = 4);

/** Compute the offset from coordinate (0,0) of the canvas to the given point */
int pointToOffset(Scroom::Utils::Point<int> p, int stride);

/**
 * Compute the offset of the point from the top-left point of the rectangle.
 * Keep in ming this is still computed with the origin (0,0)  as the absolute
 * point of reference, for both the rectangle and the point!
 */
int pointToOffset(Scroom::Utils::Rectangle<int> rect,
                  Scroom::Utils::Point<int> p);

/**
 * Compute the Rectangle (in pixels) spanned by the union of all toggled layers
 * set in the bitmap. If @param fromOrigin is true, coordinate (0,0) will be
 * used as a starting point. If not, the smallest top-left point of all set
 * layers will be used instead.
 */
Scroom::Utils::Rectangle<int>
spannedRectangle(boost::dynamic_bitset<> bitmap,
                 std::vector<SliLayer::Ptr> layers, bool fromOrigin = false);

/**
 * Finds the multiple of 2 whose size best approximates splitting @param height
 * into @param nSegments segments.
 */
int findBestSegFit(unsigned int nSegments, unsigned int height);

/**
 * Returns a bitmask that divides the @param toggledSegments bitamp in half.
 */
boost::dynamic_bitset<> halfSegBitmask(boost::dynamic_bitset<> toggledSegments);