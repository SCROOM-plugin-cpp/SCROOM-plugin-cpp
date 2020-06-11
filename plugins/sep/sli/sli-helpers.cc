#include "sli-helpers.hh"


SurfaceWrapper::Ptr SurfaceWrapper::create()
{
  SurfaceWrapper::Ptr result(new SurfaceWrapper());
  
  return result;
}

SurfaceWrapper::Ptr SurfaceWrapper::create(int width, int height, cairo_format_t format)
{
  SurfaceWrapper::Ptr result(new SurfaceWrapper(width, height, format));
  
  return result;
}

SurfaceWrapper::SurfaceWrapper()
{
  clear = true;
  empty = true;
}

SurfaceWrapper::SurfaceWrapper(int width, int height, cairo_format_t format)
{
  int stride = cairo_format_stride_for_width(format, width);
  uint8_t* bitmap = static_cast<uint8_t*>(calloc(height * stride, 1));
  surface = cairo_image_surface_create_for_data(bitmap, CAIRO_FORMAT_ARGB32, width, height, stride);
  empty = false;
  clear = true;
}

int SurfaceWrapper::getHeight()
{
  return cairo_image_surface_get_height(surface);
}

int SurfaceWrapper::getWidth()
{
  return cairo_image_surface_get_width(surface);
}

int SurfaceWrapper::getStride()
{
  return cairo_image_surface_get_stride(surface);
}

uint8_t* SurfaceWrapper::getBitmap()
{
  return cairo_image_surface_get_data(surface);
}

Scroom::Utils::Rectangle<int> SurfaceWrapper::toBytesRectangle()
{
  Scroom::Utils::Rectangle<int> rect {0, 0, getStride(), getHeight()};
  
  return rect;
}

void SurfaceWrapper::clearSurface()
{
  cairo_t* cr = cairo_create(surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_destroy(cr);
  clear = true;
}

void SurfaceWrapper::clearSurface(Scroom::Utils::Rectangle<int> rect)
{
  cairo_t* cr = cairo_create(surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_rectangle(cr, rect.getTopLeft().x, rect.getTopLeft().y, rect.getWidth(), rect.getHeight());
  cairo_fill(cr);
  cairo_destroy(cr);
  clear = true;
}

SurfaceWrapper::~SurfaceWrapper()
{
  if (!empty)
  {
    free(cairo_image_surface_get_data(surface));
    cairo_surface_destroy(surface);
  }
}