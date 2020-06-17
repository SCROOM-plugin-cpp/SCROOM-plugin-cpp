#include "sli-helpers.hh"

#include <tiffio.h>

#define TIFFGetFieldChecked(file, field, ...)        \
  if (1 != TIFFGetField(file, field, ##__VA_ARGS__)) \
    throw std::invalid_argument("Field not present in tiff file: " #field);

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

Scroom::Utils::Rectangle<int> SurfaceWrapper::toRectangle()
{
  Scroom::Utils::Rectangle<int> rect {0, 0, getWidth(), getHeight()};
  
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

// TODO bps/bpp/spp are pretty much used as magic numbers throughout the SLI code
Scroom::Utils::Rectangle<int> toBytesRectangle(Scroom::Utils::Rectangle<int> rect, int bpp)
{
  Scroom::Utils::Rectangle<int> bytesRect {rect.getLeft()*bpp, rect.getTop(), rect.getWidth()*bpp, rect.getHeight()};
  
  return bytesRect;
}

int getArea(Scroom::Utils::Rectangle<int> rect)
{
  return rect.getHeight() * rect.getWidth();
}

int pointToOffset(Scroom::Utils::Point<int> p, int stride)
{
  return p.y * stride + p.x;
}

int pointToOffset(Scroom::Utils::Rectangle<int> rect, Scroom::Utils::Point<int> p)
{
  return (p.y - rect.getTop()) * rect.getWidth() + (p.x - rect.getLeft());
}

Scroom::Utils::Rectangle<int> spannedRectangle(boost::dynamic_bitset<> bitmap, std::vector<SliLayer::Ptr> layers, bool fromOrigin)
{ 
  int min_x0 = INT_MAX;
  int min_y0 = INT_MAX;
  int max_x1 = INT_MIN;
  int max_y1 = INT_MIN;

  if (fromOrigin)
  {
    min_x0 = 0;
    min_y0 = 0;
  }

  for (size_t i = 0; i < bitmap.size(); i++)
  {
    if (!bitmap[i])
      continue;

    auto rect = layers[i]->toRectangle();

    if (rect.getLeft() < min_x0)
      min_x0 = rect.getLeft();

    if (rect.getTop() < min_y0)
      min_y0 = rect.getTop();
    
    if (rect.getRight() > max_x1)
      max_x1 = rect.getRight();
      
    if (rect.getBottom() > max_y1)
      max_y1= rect.getBottom();
  }
  
  Scroom::Utils::Rectangle<int> rect {min_x0, min_y0, max_x1 - min_x0, max_y1 - min_y0};

  return rect;
}

SurfaceWrapper::~SurfaceWrapper()
{
  if (!empty)
  {
    free(cairo_image_surface_get_data(surface));
    cairo_surface_destroy(surface);
  }
}

bool fillFromTiff(SliLayer::Ptr layer)
{
  // We only support simple CMYK Tiffs with 4 SPP and 8 BPS
  const uint16 allowedSpp = 4;
  const uint16 allowedBps = 8; 
  try
  {
    TIFF *tif = TIFFOpen(layer->filepath.c_str(), "r");
    if (!tif)
    {
      printf("PANIC: Failed to open file %s\n", layer->filepath.c_str());
      return false;
    }

    if (1 != TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &layer->spp))
      layer->spp = 1; // Default value, according to tiff spec
    if (layer->spp != allowedSpp)
    {
      printf("PANIC: Samples per pixel is not %d, but %d. Giving up\n", allowedSpp, layer->spp);
      return false;
    }

    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &layer->bps);
    if (layer->bps != allowedBps)
    {
      printf("PANIC: Bits per sample is not %d, but %d. Giving up\n", allowedBps, layer->bps);
      return false;
    }

    float resolutionX;
    float resolutionY;
    uint16 resolutionUnit;

    if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &resolutionX) &&
        TIFFGetField(tif, TIFFTAG_YRESOLUTION, &resolutionY) &&
        TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resolutionUnit))
    {
      if (resolutionUnit != RESUNIT_NONE)
      {
        // Fix aspect ratio only
        float base = std::max(resolutionX, resolutionY);
        layer->xAspect = base / resolutionX;
        layer->yAspect = base / resolutionY;
      }
    }
    else
    {
      layer->xAspect = 1.0;
      layer->yAspect = 1.0;
    }

    TIFFGetFieldChecked(tif, TIFFTAG_IMAGEWIDTH, &layer->width);
    TIFFGetFieldChecked(tif, TIFFTAG_IMAGELENGTH, &layer->height);
    printf("This bitmap has size %d*%d, aspect ratio %.1f*%.1f\n",
           layer->width, layer->height, layer->xAspect, layer->yAspect);

    // create sli bitmap ------------------------------------
    // Could just use the width here but we don't want to make overflows too easy, right ;)
    int byteWidth = TIFFScanlineSize(tif);
    layer->bitmap = static_cast<uint8_t*>(malloc(byteWidth * layer->height));
    int stride = layer->width * layer->spp;

    // Iterate over the rows and copy the bitmap data to newly allocated memory pointed to by currentBitmap
    for (int row = 0; row < layer->height; row++)
    {
      TIFFReadScanline(tif, layer->bitmap + row*stride, row);
    }
    
    TIFFClose(tif);
    return true;
  }
  catch (const std::exception &ex)
  {
    printf("PANIC: %s\n", ex.what());
    return false;
  }

}