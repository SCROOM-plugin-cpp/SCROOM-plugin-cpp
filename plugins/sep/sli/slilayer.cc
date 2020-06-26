#include "slilayer.hh"
#include "../sep-helpers.hh"
#include <boost/format.hpp>
#include <tiffio.h>

#define TIFFGetFieldChecked(file, field, ...)                                  \
  if (1 != TIFFGetField(file, field, ##__VA_ARGS__))                           \
    throw std::invalid_argument("Field not present in tiff file: " #field);

SliLayer::Ptr SliLayer::create(const std::string &filepath,
                               const std::string &name, int xoffset,
                               int yoffset) {
  SliLayer::Ptr layer(new SliLayer());
  layer->filepath = filepath;
  layer->name = name;
  layer->xoffset = xoffset;
  layer->yoffset = yoffset;
  return layer;
}

SliLayer::SliLayer() : height(0), width(0) {}

SliLayer::~SliLayer() { free(bitmap); }

Scroom::Utils::Rectangle<int> SliLayer::toRectangle() {
  Scroom::Utils::Rectangle<int> rect{xoffset, yoffset, width, height};

  return rect;
}

bool SliLayer::fillMetaFromTiff(unsigned int allowedBps,
                                unsigned int allowedSpp) {
  try {
    TIFF *tif = TIFFOpen(filepath.c_str(), "r");
    if (!tif) {
      boost::format errorFormat =
          boost::format("Error: Failed to open file %s") % filepath.c_str();
      printf("%s\n", errorFormat.str().c_str());
      Show(errorFormat.str(), GTK_MESSAGE_ERROR);
      return false;
    }

    if (1 != TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp))
      spp = 1; // Default value, according to tiff spec
    if (spp != allowedSpp) {
      boost::format errorFormat =
          boost::format(
              "Error: Samples per pixel of file %s is not %d, but %d") %
          filepath.c_str() % allowedSpp % spp;
      printf("%s\n", errorFormat.str().c_str());
      Show(errorFormat.str(), GTK_MESSAGE_ERROR);
      return false;
    }

    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
    if (bps != allowedBps) {
      boost::format errorFormat =
          boost::format("Error: Bits per sample of file %s is not %d, but %d") %
          filepath.c_str() % allowedBps % bps;
      printf("%s\n", errorFormat.str().c_str());
      Show(errorFormat.str(), GTK_MESSAGE_ERROR);
      return false;
    }

    float resolutionX;
    float resolutionY;
    uint16 resolutionUnit;

    if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &resolutionX) &&
        TIFFGetField(tif, TIFFTAG_YRESOLUTION, &resolutionY) &&
        TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resolutionUnit)) {
      if (resolutionUnit != RESUNIT_NONE) {
        // Fix aspect ratio only
        float base = std::max(resolutionX, resolutionY);
        xAspect = base / resolutionX;
        yAspect = base / resolutionY;
      }
    } else {
      xAspect = 1.0;
      yAspect = 1.0;
    }

    TIFFGetFieldChecked(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetFieldChecked(tif, TIFFTAG_IMAGELENGTH, &height);
    printf("This bitmap has size %d*%d, aspect ratio %.1f*%.1f\n", width,
           height, xAspect, yAspect);

    TIFFClose(tif);
    return true;

  } catch (const std::exception &ex) {
    boost::format errorFormat = boost::format("Error: %s") % ex.what();
    printf("%s\n", errorFormat.str().c_str());
    Show(errorFormat.str(), GTK_MESSAGE_ERROR);
    return false;
  }
}

void SliLayer::fillBitmapFromTiff() {
  try {
    TIFF *tif = TIFFOpen(filepath.c_str(), "r");
    if (!tif) {
      boost::format errorFormat =
          boost::format("Error: Failed to open file %s") % filepath.c_str();
      printf("%s\n", errorFormat.str().c_str());
      Show(errorFormat.str(), GTK_MESSAGE_ERROR);
      return;
    }

    // create sli bitmap ------------------------------------
    // Could just use the width here but we don't want to make overflows too
    // easy, right ;)
    int byteWidth = TIFFScanlineSize(tif);
    bitmap = static_cast<uint8_t *>(malloc(byteWidth * height));
    int stride = width * spp;

    // Iterate over the rows and copy the bitmap data to newly allocated memory
    // pointed to by currentBitmap
    for (int row = 0; row < height; row++) {
      TIFFReadScanline(tif, bitmap + row * stride, row);
    }

    TIFFClose(tif);

  } catch (const std::exception &ex) {
    boost::format errorFormat = boost::format("Error: %s") % ex.what();
    printf("%s\n", errorFormat.str().c_str());
    Show(errorFormat.str(), GTK_MESSAGE_ERROR);
  }
}
