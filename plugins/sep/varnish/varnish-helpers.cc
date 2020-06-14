#include "varnish-helpers.hh"
#include <scroom/cairo-helpers.hh>
#include <tiffio.h>

#define TIFFGetFieldChecked(file, field, ...)        \
  if (1 != TIFFGetField(file, field, ##__VA_ARGS__)) \
    throw std::invalid_argument("Field not present in tiff file: " #field)

void redrawVarnishOverlay(SliLayer::Ptr layer, ViewInterface::Ptr const &vi, cairo_t *cr,
              Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, layer->width);
  cairo_surface_t* varnishSurface = cairo_image_surface_create_for_data(layer->bitmap, CAIRO_FORMAT_ARGB32, 
                                                                    layer->width, layer->height, stride);
  double pixelSize = pixelSizeFromZoom(zoom);
  GdkRectangle GTKPresArea = presentationArea.toGdkRectangle();
  cairo_save(cr);
  /* TODO;  this line causes segfaults when rapidly panning. not sure why 
            Could also be a timing issue?
  */
  cairo_translate(cr, -GTKPresArea.x*pixelSize,-GTKPresArea.y*pixelSize);
  if(zoom >= 0)
  {
    cairo_scale(cr, 1<<zoom, 1<<zoom);
  } else {
    cairo_scale(cr, 1<<-zoom, 1<<-zoom);
  }
  cairo_set_source_surface(cr, varnishSurface, 0, 0);
  cairo_paint_with_alpha(cr, 0.6); // Should be set to 1 when background is properly set
  cairo_surface_destroy(varnishSurface);
  cairo_restore(cr);
}


/*
    TODO; This copies quite a lot of code from sli-helpers.
    In theory, we only need to change 3 things here:
        * allowedSpp
        * allowedBps
        * How bits are modified before they are saved
*/
void fillVarnishOverlay(SliLayer::Ptr layer)
{
  // We only support simple K Tiffs with 1 SPP and 8 BPS
  const uint16 allowedSpp = 1;
  const uint16 allowedBps = 8; 
  try
  {
    TIFF *tif = TIFFOpen(layer->filepath.c_str(), "r");
    if (!tif)
    {
      printf("PANIC: Failed to open file %s\n", layer->filepath.c_str());
      return;
    }

    if (1 != TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &layer->spp))
      layer->spp = 1; // Default value, according to tiff spec
    if (layer->spp != allowedSpp)
    {
      printf("PANIC: Samples per pixel is not %d, but %d. Giving up\n", allowedSpp, layer->spp);
      return;
    }

    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &layer->bps);
    if (layer->bps != allowedBps)
    {
      printf("PANIC: Bits per sample is not %d, but %d. Giving up\n", allowedBps, layer->bps);
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
      //TODO, add a fluorecent color here
      //TODO, add tranparency here
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