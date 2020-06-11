#pragma once

#include <cairo.h>

#include <scroom/utilities.hh>
#include <scroom/rectangle.hh>
#include <scroom/point.hh>

class SurfaceWrapper: public virtual Scroom::Utils::Base
{
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
  /** Constructor */
  static Ptr create();
  static Ptr create(int width, int height, cairo_format_t format);

  /** Get the height of the wrapped surface */
  virtual int getHeight();

  /** Get the width of the wrapped surface */
  virtual int getWidth();

  /** Get the stride of the wrapped surface */
  virtual int getStride();

  /** Get the bitmap of the wrapped surface */
  virtual uint8_t* getBitmap();

  /** Fill the entire surface with 0s */
  virtual void clearSurface();

  /** Fill a rectangle of the surface with 0s */
  virtual void clearSurface(Scroom::Utils::Rectangle<int> rect);

  /** Return the Rectangle representation of the surface*/
  virtual Scroom::Utils::Rectangle<int> toBytesRectangle();

  /** Destructor */
  virtual ~SurfaceWrapper();
};

/** Compute area in pixels of the given rectangle */
inline int getArea(Scroom::Utils::Rectangle<int> rect)
{
  return rect.getHeight() * rect.getWidth();
}

/** Compute the offset from coordinate (0,0) of the canvas to the given point */
inline int pointToOffset(Scroom::Utils::Point<int> p, int stride)
{
  return p.y * stride + p.x;
}

/** Compute the offset of the point from the top-left point of the rectangle */
inline int pointToOffset(Scroom::Utils::Rectangle<int> rect, Scroom::Utils::Point<int> p)
{
  return (p.y - rect.getTop()) * rect.getWidth() + (p.x - rect.getLeft());
}