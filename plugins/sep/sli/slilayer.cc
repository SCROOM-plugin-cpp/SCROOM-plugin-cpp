#include "slilayer.hh"
#include "../colorconfig/CustomColorConfig.hh"
#include "../sep-helpers.hh"
#include <fmt/format.h>
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

Scroom::Utils::Rectangle<int> SliLayer::toRectangle() {
  Scroom::Utils::Rectangle<int> rect{xoffset, yoffset, width, height};

  return rect;
}

bool SliLayer::fillMetaFromTiff(unsigned int allowedBps,
                                unsigned int allowedSpp) {
  try {
    TIFF *tif = TIFFOpen(filepath.c_str(), "r");
    if (!tif) {
      auto error = fmt::format("Error: Failed to open file {}", filepath);
      printf("%s\n", error.c_str());
      Show(error, GTK_MESSAGE_ERROR);
      return false;
    }

    if (1 != TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp))
      spp = 1; // Default value, according to tiff spec
    if (spp != allowedSpp) {
      auto error =
          fmt::format("Error: Samples per pixel of file {} is not {}, but {}",
                      filepath, allowedSpp, spp);
      printf("%s\n", error.c_str());
      Show(error, GTK_MESSAGE_ERROR);
      return false;
    }

    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
    if (bps != allowedBps) {
      auto error =
          fmt::format("Error: Bits per sample of file {} is not {}, but {}",
                      filepath, allowedBps, bps);
      printf("%s\n", error.c_str());
      Show(error, GTK_MESSAGE_ERROR);
      return false;
    }

    float resolutionX;
    float resolutionY;
    uint16_t resolutionUnit;

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
    channels = {ColorConfig::getInstance().getColorByNameOrAlias("c"),
                ColorConfig::getInstance().getColorByNameOrAlias("m"),
                ColorConfig::getInstance().getColorByNameOrAlias("y"),
                ColorConfig::getInstance().getColorByNameOrAlias("k")};
    return true;

  } catch (const std::exception &ex) {
    auto error = fmt::format("Error: {}", ex.what());
    printf("%s\n", error.c_str());
    Show(error, GTK_MESSAGE_ERROR);
    return false;
  }
}

void SliLayer::fillBitmapFromTiff() {
  try {
    TIFF *tif = TIFFOpen(filepath.c_str(), "r");
    if (!tif) {
      auto error = fmt::format("Error: Failed to open file {}", filepath);
      printf("%s\n", error.c_str());
      Show(error, GTK_MESSAGE_ERROR);
      return;
    }

    // create sli bitmap ------------------------------------
    // Could just use the width here but we don't want to make overflows too
    // easy, right ;)
    int byteWidth = TIFFScanlineSize(tif);
    bitmap.reset(new uint8_t[byteWidth * height]);
    int stride = width * spp;

    // Iterate over the rows and copy the bitmap data to newly allocated memory
    // pointed to by currentBitmap
    for (int row = 0; row < height; row++) {
      TIFFReadScanline(tif, &bitmap[row * stride], row);
    }

    TIFFClose(tif);

  } catch (const std::exception &ex) {
    auto error = fmt::format("Error: {}", ex.what());
    printf("%s\n", error.c_str());
    Show(error, GTK_MESSAGE_ERROR);
  }
}
