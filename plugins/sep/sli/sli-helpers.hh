#include <cairo.h>

#include <scroom/utilities.hh>

class SurfaceWrapper: public virtual Scroom::Utils::Base
{
public:
  typedef boost::shared_ptr<SurfaceWrapper> Ptr;

  /** The cairo surface wrapped by this class */
  cairo_surface_t *surface;

private:
  SurfaceWrapper(int width, int height, cairo_format_t format);

public:
  /** Constructor */
  static Ptr create(int width, int height, cairo_format_t format);

  /** Get the height of the wrapped surface */
  virtual int getHeight();

  /** Get the width of the wrapped surface */
  virtual int getWidth();

  /** Get the stride of the wrapped surface */
  virtual int getStride();

  /** Get the bitmap of the wrapped surface */
  virtual uint8_t* getBitmap();

  /** Destructor */
  virtual ~SurfaceWrapper();
};