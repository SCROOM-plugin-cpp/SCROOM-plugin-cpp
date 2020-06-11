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

// TODO set the instance bps, spp values too at some point
void fillFromTiff(SliLayer::Ptr layer)
{
  try
  {
    TIFF *tif = TIFFOpen(layer->filepath.c_str(), "r");
    if (!tif)
    {
      printf("PANIC: Failed to open file %s\n", layer->filepath.c_str());
      return;
    }

    uint16 spp_ = 0;
    if (1 != TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp_))
      spp_ = 1; // Default value, according to tiff spec
    if (spp_ != SPP)
    {
      printf("PANIC: Samples per pixel is not %d, but %d. Giving up\n", SPP, spp_);
      return;
    }

    uint16 bps_ = 0;
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps_);
    if (bps_ != BPS)
    {
      printf("PANIC: Bits per sample is not %d, but %d. Giving up\n", BPS, bps_);
      return;
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
        resolutionX = resolutionX / base;
        resolutionY = resolutionY / base;
      }
    }
    else
    {
      resolutionX = 1;
      resolutionY = 1;
    }

    TIFFGetFieldChecked(tif, TIFFTAG_IMAGEWIDTH, &layer->width);
    TIFFGetFieldChecked(tif, TIFFTAG_IMAGELENGTH, &layer->height);
    printf("This bitmap has size %d*%d, aspect ratio %.1f*%.1f\n",
           layer->width, layer->height, 1 / resolutionX, 1 / resolutionY);

    // create sli bitmap ------------------------------------
    // Could just use the width here but we don't want to make overflows too easy, right ;)
    int byteWidth = TIFFScanlineSize(tif);
    layer->bitmap = static_cast<uint8_t*>(malloc(byteWidth * layer->height));
    int stride = layer->width*SPP;

    // Iterate over the rows and copy the bitmap data to newly allocated memory pointed to by currentBitmap
    for (int row = 0; row < layer->height; row++)
    {
      TIFFReadScanline(tif, layer->bitmap + row*stride, row);
    }
    
    TIFFClose(tif);
  }
  catch (const std::exception &ex)
  {
    printf("PANIC: %s\n", ex.what());
    return;
  }

}