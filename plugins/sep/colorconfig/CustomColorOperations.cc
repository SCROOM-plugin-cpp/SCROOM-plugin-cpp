//
// Created by jelle on 02/06/2021.
//

#include "CustomColorOperations.hh"
#include "CustomColorHelpers.hh"
#include <iostream>
#include <scroom/bitmap-helpers.hh>
#include <utility>

boost::shared_ptr<unsigned char> shared_malloc(size_t size) {
  return boost::shared_ptr<unsigned char>(
      static_cast<unsigned char *>(malloc(size)), free);
}

PipetteLayerOperations::PipetteColor
PipetteCommonOperationsCustomColor::sumPixelValues(
    Scroom::Utils::Rectangle<int> area, const ConstTile::Ptr tile) {
  int offset = spp * (area.getTop() * tile->width + area.getLeft());
  int stride = spp * (tile->width - area.getWidth());
  Scroom::Bitmap::SampleIterator<const uint8_t> si(tile->data.get(), 0, bps);
  si += offset;

  std::vector<size_t> sums = {};

  // Initialize sums of all colors to 0
  for (int i = 0; i < spp; i++) {
    sums.push_back(0);
  }

  for (int y = area.getTop(); y < area.getBottom(); y++) {
    for (int x = area.getLeft(); x < area.getRight(); x++) {
      for (int i = 0; i < spp; i++) {
        sums[i] += *si++;
      }
    }
    si += stride;
  }

  // Copy map to vector of pairs
  PipetteColor result = {};
  // Map different aliasses of the same color to the same pipette color
  std::unordered_map<std::string, int> colorIndex = {};
  for (int i = 0; i < spp; i++) {
    // if the color is already in the pipette colors, add the sum to that color
    if (colorIndex.find(colors[i]->name) != colorIndex.end()) {
      result[colorIndex[colors[i]->name]].second += sums[i];
    }
    // This is a pipette color that we have not seen before
    else {
      // The pipette color that is about to be added will have the index of the
      // current size, as vectors are 0 indexed
      colorIndex.insert({colors[i]->name, result.size()});
      result.push_back(
          std::pair<std::string, double>(colors[i]->name, sums[i]));
    }
  }

  return result;
}

OperationsCustomColors::OperationsCustomColors(int spp_)
    : PipetteCommonOperationsCustomColor(8, spp_) {}

PipetteCommonOperationsCustomColor::Ptr
OperationsCustomColors::create(int spp) {
  return PipetteCommonOperationsCustomColor::Ptr(
      new OperationsCustomColors(spp));
}

Scroom::Utils::Stuff OperationsCustomColors::cache(const ConstTile::Ptr tile) {
  // Allocate the space for the cache - stride is the height of one row
  const int stride =
      cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, tile->width);
  boost::shared_ptr<uint8_t> data = shared_malloc(stride * tile->height);

  // Row is a pointer to a row of pixels (destination)
  auto *row = reinterpret_cast<uint32_t *>(data.get());
  // Cur is a pointer to the start of the row in the tile (source)
  const uint8_t *cur = tile->data.get();

  for (int i = 0; i < spp * tile->height * tile->width; i += spp) {
    // Convert custom colors to CMYK and then to ARGB, because cairo doesn't
    // know how to render CMYK.
    int16_t C = 0;
    int16_t M = 0;
    int16_t Y = 0;
    int16_t K = 0;
    for (uint16_t j = 0; j < spp; j++) {
      auto &color = colors[j];
      CustomColorHelpers::calculateCMYK(color, C, M, Y, K, cur[i + j]);
    }

    uint8_t C_i = 255 - CustomColorHelpers::toUint8(C);
    uint8_t M_i = 255 - CustomColorHelpers::toUint8(M);
    uint8_t Y_i = 255 - CustomColorHelpers::toUint8(Y);
    uint8_t K_i = 255 - CustomColorHelpers::toUint8(K);

    uint8_t R = (C_i * K_i) / 255;
    uint8_t G = (M_i * K_i) / 255;
    uint8_t B = (Y_i * K_i) / 255;

    // Write 255 as alpha (fully opaque)
    uint32_t target = i * 4 / spp; // Scale the target to the 4 channel target
                                   // row, from the n channel source row
    row[target / 4] = 255u << 24 | R << 16 | G << 8 | B;
  }
  return Scroom::Bitmap::BitmapSurface::create(
      tile->width, tile->height, CAIRO_FORMAT_ARGB32, stride, data);
}

void OperationsCustomColors::reduce(Tile::Ptr target,
                                    const ConstTile::Ptr source, int top_left_x,
                                    int top_left_y) {
  // Reducing by a factor 8
  int sourceStride = getBpp() * source->width / 8; // stride in bytes
  const byte *sourceBase = source->data.get();

  int targetStride = getBpp() * target->width / 8; // stride in bytes
  byte *targetBase =
      target->data.get() +
      (target->height * top_left_y + top_left_x) * targetStride / 8;

  for (int y = 0; y < source->height / 8; y++) {
    byte *targetPtr = targetBase;

    for (int x = 0; x < source->width / 8; x++) {
      // We want to store the average colour of the 8*8 pixel image
      // with (x, y) as its top-left corner into targetPtr.
      const byte *base = sourceBase + 8 * spp * x; // start of the row
      const byte *end = base + 8 * sourceStride;   // end of the row

      // Initialize sums of all colors to 0
      std::vector<size_t> sums(spp, 0);

      for (const byte *row = base; row < end;
           row += sourceStride) // Iterate over rows
      {
        for (size_t current = 0; current < 8 * spp;
             current += spp) // Iterate over pixels
        {
          for (uint16_t i = 0; i < spp; i++) { // Iterate over samples in pixel
            sums[i] += row[current + i];
          }
        }
      }

      // Calculate and store the average in the target
      for (uint16_t i = 0; i < spp; i++) {
        targetPtr[i] = static_cast<byte>(sums[i] / 64);
      }

      targetPtr += spp;
    }

    targetBase += targetStride;
    sourceBase += sourceStride * 8;
  }
}

int OperationsCustomColors::getBpp() { return spp * bps; }

void PipetteCommonOperationsCustomColor::setColors(
    std::vector<CustomColor::Ptr> colors_) {
  colors = std::move(colors_);
}
