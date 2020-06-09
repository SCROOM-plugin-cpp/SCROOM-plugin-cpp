#include "sli-helpers.hh"

SurfaceWrapper::Ptr SurfaceWrapper::create(int width, int height, cairo_format_t format)
{
  SurfaceWrapper::Ptr result(new SurfaceWrapper(width, height, format));
  
  return result;
}

SurfaceWrapper::SurfaceWrapper(int width, int height, cairo_format_t format)
{
  int stride = cairo_format_stride_for_width(format, width);
  uint8_t* bitmap = static_cast<uint8_t*>(calloc(height * stride, 1));
  surface = cairo_image_surface_create_for_data(bitmap, CAIRO_FORMAT_ARGB32, width, height, stride);
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

SurfaceWrapper::~SurfaceWrapper()
{
  free(cairo_image_surface_get_data(surface));
  cairo_surface_destroy(surface);
}