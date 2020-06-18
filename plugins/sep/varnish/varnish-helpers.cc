#include "varnish-helpers.hh"
#include <scroom/cairo-helpers.hh>
#include <tiffio.h>

#define TIFFGetFieldChecked(file, field, ...)        \
  if (1 != TIFFGetField(file, field, ##__VA_ARGS__)) \
    throw std::invalid_argument("Field not present in tiff file: " #field)

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