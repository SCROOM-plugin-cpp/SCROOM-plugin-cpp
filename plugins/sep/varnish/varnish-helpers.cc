#include "varnish-helpers.hh"
#include "../sep-helpers.hh"
#include <scroom/cairo-helpers.hh>

#include <tiffio.h>
#include <boost/format.hpp>

#define TIFFGetFieldChecked(file, field, ...)        \
  if (1 != TIFFGetField(file, field, ##__VA_ARGS__)) \
    throw std::invalid_argument("Field not present in tiff file: " #field)

bool fillVarnishOverlay(SliLayer::Ptr layer)
{
  // We only support simple K Tiffs with 1 SPP and 8 BPS
  const uint16 allowedSpp = 1;
  const uint16 allowedBps = 8; 
  try
  {
    TIFF *tif = TIFFOpen(layer->filepath.c_str(), "r");
    if (!tif)
    {
      boost::format errorFormat = boost::format(
          "Error: Failed to open file %s") % layer->filepath.c_str();
      printf("%s\n", errorFormat.str().c_str());
      Show(errorFormat.str(), GTK_MESSAGE_ERROR);
      return false;
    }

    if (1 != TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &layer->spp))
      layer->spp = 1; // Default value, according to tiff spec
    if (layer->spp != allowedSpp)
    {
      boost::format errorFormat = boost::format(
          "Error: Samples per pixel of file %s is not %d, but %d") 
          % layer->filepath.c_str() % allowedSpp % layer->spp;
      printf("%s\n", errorFormat.str().c_str());
      Show(errorFormat.str(), GTK_MESSAGE_ERROR);
      return false;
    }

    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &layer->bps);
    if (layer->bps != allowedBps)
    {
      boost::format errorFormat = boost::format(
          "Error: Bits per sample of file %s is not %d, but %d") 
          % layer->filepath.c_str() % allowedBps % layer->bps;
      printf("%s\n", errorFormat.str().c_str());
      Show(errorFormat.str(), GTK_MESSAGE_ERROR);
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
  }
  catch (const std::exception &ex)
  {
    boost::format errorFormat = boost::format("Error: %s") % ex.what();
    printf("%s\n", errorFormat.str().c_str());
    Show(errorFormat.str(), GTK_MESSAGE_ERROR);
    return false;
  }

  // Till Fill was successful
  return true;
}