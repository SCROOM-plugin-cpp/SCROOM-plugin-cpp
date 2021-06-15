#include "slisource.hh"
#include "../sep-helpers.hh"
#include "../sepsource.hh"
#include "../colorconfig/CustomColorHelpers.hh"

#include <scroom/bitmap-helpers.hh>

#include <boost/format.hpp>

SliSource::SliSource(boost::function<void()> &triggerRedrawFunc)
    : triggerRedraw(triggerRedrawFunc) {
  threadQueue = ThreadPool::Queue::create();
}

SliSource::~SliSource() {}

SliSource::Ptr SliSource::create(boost::function<void()> &triggerRedrawFunc) {
  return Ptr(new SliSource(triggerRedrawFunc));
}

void SliSource::computeHeightWidth() {
  auto rect = spannedRectangle(toggled, layers, true);
  total_width = rect.getWidth();
  total_height = rect.getHeight();
}

void SliSource::checkXoffsets() {
  hasXoffsets = false;
  for (auto layer : layers) {
    if (layer->xoffset != 0)
      hasXoffsets = true;
  }
}

bool SliSource::addLayer(std::string imagePath, std::string filename,
                         int xOffset, int yOffset) {

  auto extension = filename.substr(filename.find_last_of("."));
  boost::to_lower(extension);

  SliLayer::Ptr layer = SliLayer::create(imagePath, filename, xOffset, yOffset);

  if (extension == ".sep") {
    sepSources[layer] = SepSource::create();
    sepSources[layer]->fillSliLayerMeta(layer);
  } else if (extension == ".tif" || extension == ".tiff") {
    if (!layer->fillMetaFromTiff(8, 4)) {
      return false;
    }
  } else {
    boost::format errorFormat =
        boost::format("Error: File extension of %s is not supported") %
        filename.c_str();
    printf("%s\n", errorFormat.str().c_str());
    Show(errorFormat.str(), GTK_MESSAGE_ERROR);
    return false;
  }
  layers.push_back(layer);
  return true;
}

void SliSource::importBitmaps() {
  for (SliLayer::Ptr layer : layers) {
    auto extension = layer->name.substr(layer->name.find_last_of("."));
    boost::to_lower(extension);

    if (extension == ".sep") {
      sepSources[layer]->fillSliLayerBitmap(layer);
    } else {
      layer->fillBitmapFromTiff();
    }
  }
  bitmapsImported = true;
  triggerRedraw();
}

void SliSource::queryImportBitmaps() {
  CpuBound()->schedule(
      boost::bind(&SliSource::importBitmaps, shared_from_this<SliSource>()),
      PRIO_HIGHER, threadQueue);
}

void SliSource::wipeCacheAndRedraw() {
  clearBottomSurface();
  getSurface(0); // recompute bottom surface and trigger redraw when ready
}

SurfaceWrapper::Ptr SliSource::getSurface(int zoom) {
  if (!bitmapsImported) {
    return nullptr;
  } else if (!rgbCache.count(std::min(0, zoom)) || rgbCache[0]->clear) {
    CpuBound()->schedule(
        boost::bind(&SliSource::fillCache, shared_from_this<SliSource>()),
        PRIO_HIGHER, threadQueue);
    if (rgbCache.count(std::min(0, zoom))) {
      return rgbCache[std::min(0, zoom)];
    }
    return nullptr;
  } else {
    return rgbCache[std::min(0, zoom)];
  }
}
void SliSource::fillCache() {
  mtx.lock();
  disableInteractions();
  visible ^= toggled;

  if (!rgbCache.count(0) || rgbCache[0]->clear) {
    computeRgb();

    for (int i = -1; i >= -30; i--) {
      // true -> uses multithreading
      reduceRgb(i, true);
    }
  }

  rgbCache[0]->clear = false;
  toggled.reset();
  enableInteractions();
  mtx.unlock();
  triggerRedraw();
}

void SliSource::reduceSegments(SurfaceWrapper::Ptr targetSurface,
                               boost::dynamic_bitset<> toggledSegments,
                               int baseSegHeight, int zoom) {
  for (int i = 0; i < (int)toggledSegments.size(); i++) {
    if (!toggledSegments[i])
      continue;

    const int sourceWidth = total_width / pow(2, -zoom - 1);
    const int sourceOffset = (baseSegHeight / pow(2, -zoom - 1)) * i;
    const int sourceStride = rgbCache[zoom + 1]->getStride();

    int targetSegHeight;
    if (i == (int)toggledSegments.size() - 1) {
      // the last segment will probably have leftover pixels
      int leftoverPixels = (total_height % baseSegHeight) / pow(2, -zoom);
      targetSegHeight = (baseSegHeight / pow(2, -zoom)) + leftoverPixels;
    } else {
      targetSegHeight = baseSegHeight / pow(2, -zoom);
    }

    const int targetWidth = sourceWidth / 2;
    const int targetOffset = (baseSegHeight / pow(2, -zoom)) * i;
    auto targetStride = targetSurface->getStride();
    auto targetBitmap =
        targetSurface->getBitmap() + targetOffset * targetStride;

    for (int y = 0; y < targetSegHeight; y++) {
      auto sourceBitmap1 = rgbCache[zoom + 1]->getBitmap() +
                           2 * y * sourceStride + sourceOffset * sourceStride;
      auto sourceBitmap2 = sourceBitmap1 + sourceStride;

      for (int x = 0; x < targetWidth; x++) {
        targetBitmap[0] = (sourceBitmap1[0] + sourceBitmap1[4] +
                           sourceBitmap2[0] + sourceBitmap2[4]) /
                          4;
        targetBitmap[1] = (sourceBitmap1[1] + sourceBitmap1[5] +
                           sourceBitmap2[1] + sourceBitmap2[5]) /
                          4;
        targetBitmap[2] = (sourceBitmap1[2] + sourceBitmap1[6] +
                           sourceBitmap2[2] + sourceBitmap2[6]) /
                          4;
        targetBitmap[3] = (sourceBitmap1[3] + sourceBitmap1[7] +
                           sourceBitmap2[3] + sourceBitmap2[7]) /
                          4;

        targetBitmap += 4;
        sourceBitmap1 += 8;
        sourceBitmap2 += 8;
      }
    }
  }
}

void SliSource::reduceRgb(int zoom, bool multithreading) {
  // the total width of the reduced image
  const int totalTargetWidth = total_width / pow(2, -zoom);
  // the total height of the reduced image
  const int totalTargetHeight = total_height / pow(2, -zoom);

  // create a new surface for this zoom level, unless it already exists
  SurfaceWrapper::Ptr targetSurface = SurfaceWrapper::create();
  if (rgbCache.count(zoom)) {
    targetSurface = rgbCache[zoom];
  } else {
    targetSurface = SurfaceWrapper::create(totalTargetWidth, totalTargetHeight,
                                           CAIRO_FORMAT_ARGB32);
  }

  unsigned int nSegments = 24; // just an arbitrary choice
  int baseSegHeight = findBestSegFit(nSegments, total_height);
  nSegments = (unsigned int)(total_height / baseSegHeight);
  boost::dynamic_bitset<> toggledSegments{nSegments};
  auto spanRect = spannedRectangle(toggled, layers);

  // find which segments intersect with the rectangle spanned by the toggled
  // layers
  int n = 0;
  for (int i = 0; i < (int)nSegments; i++) {
    Scroom::Utils::Rectangle<int> seg = {0, baseSegHeight * i, total_width,
                                         baseSegHeight};
    if (seg.intersects(spanRect)) {
      toggledSegments.set(i);
      n++;
    }
  }

  // since all segments are disjoint, we can assign one thread to cover half of
  // them
  if (multithreading && (n / (double)nSegments) >= 0.25) {
    auto bitmask = halfSegBitmask(toggledSegments);
    auto toggledSegments1 = toggledSegments & bitmask;
    auto toggledSegments2 = toggledSegments & ~bitmask;
    boost::thread thread(
        boost::bind(&SliSource::reduceSegments, shared_from_this<SliSource>(),
                    targetSurface, toggledSegments2, baseSegHeight, zoom));

    // reduce the segments and copy them over to the targetSurface
    reduceSegments(targetSurface, toggledSegments1, baseSegHeight, zoom);

    thread.join();
  } else {
    reduceSegments(targetSurface, toggledSegments, baseSegHeight, zoom);
  }

  if (!rgbCache.count(zoom))
    rgbCache[zoom] = targetSurface;
}

void SliSource::convertCmykXoffset(uint8_t *surfacePointer,
                                   uint32_t *targetPointer, int topLeftOffset,
                                   int bottomRightOffset, int toggledWidth,
                                   int toggledBound, int stride) {
  double black;
  uint8_t C, M, Y, K, A, R, G, B;

  for (int i = topLeftOffset; i < bottomRightOffset;) {
    C = surfacePointer[i + 0];
    M = surfacePointer[i + 1];
    Y = surfacePointer[i + 2];
    K = surfacePointer[i + 3];

    black = (1 - K / 255.0);
    A = 255;
    R = 255 * (1 - C / 255.0) * black;
    G = 255 * (1 - M / 255.0) * black;
    B = 255 * (1 - Y / 255.0) * black;

    targetPointer[i / 4] = (A << 24) | (R << 16) | (G << 8) | B;
    i += 4; // SPP = 4

    // we are past the image bounds; go to the next next line
    if (i % stride == toggledBound) {
      i += stride - toggledWidth;
    }
  }
}

void SliSource::convertCmyk(uint8_t *surfacePointer, uint32_t *targetPointer,
                            int topLeftOffset, int bottomRightOffset) {
  double black;
  uint8_t C, M, Y, K, A, R, G, B;

  for (int i = topLeftOffset; i < bottomRightOffset; i += 4) // SPP = 4
  {
    C = surfacePointer[i + 0];
    M = surfacePointer[i + 1];
    Y = surfacePointer[i + 2];
    K = surfacePointer[i + 3];

    black = (1 - K / 255.0);
    A = 255;
    R = 255 * (1 - C / 255.0) * black;
    G = 255 * (1 - M / 255.0) * black;
    B = 255 * (1 - Y / 255.0) * black;

    targetPointer[i / 4] = (A << 24) | (R << 16) | (G << 8) | B;
  }
}

void SliSource::drawCmyk(uint8_t *surfacePointer, uint8_t *bitmap,
                         int bitmapStart, int bitmapOffset, SliLayer::Ptr layer) {

  for (int i = bitmapStart; i < bitmapStart + bitmapOffset; i+= layer->spp) { // Iterate over all pixels
    int32_t C = *surfacePointer; // Initialize the CMYK holder values to the current values for their color
    int32_t M = *(surfacePointer+1);
    int32_t Y = *(surfacePointer+2);
    int32_t K = *(surfacePointer+3);
    for (int j = 0; j < layer->spp; j++) { // Add values to the 32bit cmyk holders
        auto color = layer->channels.at(j);
        C += color->cMultiplier * static_cast<float>(bitmap[i + j]);
        M += color->mMultiplier * static_cast<float>(bitmap[i + j]);
        Y += color->yMultiplier * static_cast<float>(bitmap[i + j]);
        K += color->kMultiplier * static_cast<float>(bitmap[i + j]);

    }
    *surfacePointer = CustomColorHelpers::toUint8(C); // Store the CMYK values back into the surface, clipped to uint_8
    *(surfacePointer+1) = CustomColorHelpers::toUint8(M);
    *(surfacePointer+2) = CustomColorHelpers::toUint8(Y);
    *(surfacePointer+3) = CustomColorHelpers::toUint8(K);

    surfacePointer += 4; // Advance the pointer
  }
}

void SliSource::drawCmykXoffset(uint8_t *surfacePointer, uint8_t *bitmap,
                                int bitmapStart, int bitmapOffset,
                                Scroom::Utils::Rectangle<int> layerRect,
                                Scroom::Utils::Rectangle<int> intersectRect,
                                int layerBound, int stride, SliLayer::Ptr layer) {
  for (int i = bitmapStart; i < bitmapStart + bitmapOffset;) {
      int k = i;
      std::vector<uint8_t*> addresses = {};
      addresses.push_back(surfacePointer); // Store the address, so it can later be written to
      int32_t C = *surfacePointer; // Initialize the CMYK holder values to the current values for their color
      // Advance k and the surface pointer, while keeping the the layer bounds in mind
      advanceIAndSurfacePointer(layerRect, intersectRect, layerBound, stride, surfacePointer, k);

      addresses.push_back(surfacePointer); // Store the m address
      int32_t M = *surfacePointer; // Surface pointer has been advanced, so now the M value can be loaded
      advanceIAndSurfacePointer(layerRect, intersectRect, layerBound, stride, surfacePointer, k);

      addresses.push_back(surfacePointer);
      int32_t Y = *surfacePointer;
      advanceIAndSurfacePointer(layerRect, intersectRect, layerBound, stride, surfacePointer, k);

      addresses.push_back(surfacePointer);
      int32_t K = *surfacePointer;
      advanceIAndSurfacePointer(layerRect, intersectRect, layerBound, stride, surfacePointer, k);


      for (int j = 0; j < layer->spp; j++) { // Add values to the 32bit cmyk holders
          auto color = layer->channels.at(j);
          C += color->cMultiplier * static_cast<float>(bitmap[i + j]);
          M += color->mMultiplier * static_cast<float>(bitmap[i + j]);
          Y += color->yMultiplier * static_cast<float>(bitmap[i + j]);
          K += color->kMultiplier * static_cast<float>(bitmap[i + j]);

      }
      // Write the CMYK values back to the surface, clipped to uint_8

      *(addresses.at(0)) = CustomColorHelpers::toUint8(C);
      *(addresses.at(1)) = CustomColorHelpers::toUint8(M);
      *(addresses.at(2)) = CustomColorHelpers::toUint8(Y);
      *(addresses.at(3)) = CustomColorHelpers::toUint8(K);
      // set i to the incremented value
      i = k;

  }
}

void SliSource::advanceIAndSurfacePointer(const Scroom::Utils::Rectangle<int> &layerRect,
                                          const Scroom::Utils::Rectangle<int> &intersectRect, int layerBound,
                                          int stride, uint8_t *&surfacePointer,
                                          int &i) const {
    // we are past the image bounds; go to the next next line
    if (i % layerRect.getWidth() == layerBound) {
      surfacePointer += stride - intersectRect.getWidth();
      i += layerRect.getWidth() - intersectRect.getWidth();
    }
}


void SliSource::computeRgb() {
  SurfaceWrapper::Ptr surface = SurfaceWrapper::create();

  // Check if cache surface exists first
  if (rgbCache.count(0)) {
    surface = rgbCache[0];
  } else {
    surface =
        SurfaceWrapper::create(total_width, total_height, CAIRO_FORMAT_ARGB32);
  }
  const int stride = surface->getStride();
  cairo_surface_flush(surface->surface);
  uint8_t *surfaceBegin = cairo_image_surface_get_data(surface->surface);
  uint32_t *targetBegin = reinterpret_cast<uint32_t *>(surfaceBegin);
  uint8_t *currentSurfaceByte = surfaceBegin;

  // Rectangle (in bytes) of the toggled area
  Scroom::Utils::Rectangle<int> toggledRect =
      toBytesRectangle(spannedRectangle(toggled, layers));

  for (size_t j = 0; j < layers.size(); j++) { // For every layer
    if (!visible[j])
      continue;

    auto layer = layers[j];
    auto bitmap = layer->bitmap.get();
    Scroom::Utils::Rectangle<int> layerRect =
        toBytesRectangle(layer->toRectangle());

    if (!layerRect.intersects(toggledRect))
      continue;

    // Rectangle area (in bytes) of the intersection between the toggled and
    // current rectangles
    Scroom::Utils::Rectangle<int> intersectRect =
        toggledRect.intersection(layerRect);
    // index of the first pixel that needs to be drawn
    int bitmapStart = pointToOffset(layerRect, intersectRect.getTopLeft());
    // offset of the last pixel from bitmapStart
    int bitmapOffset = intersectRect.getHeight() * layerRect.getWidth();
    // offset of the surface pointer from the top-left point of the surface
    int surfacePointerOffset =
        pointToOffset(intersectRect.getTopLeft(), stride);
    currentSurfaceByte = surfaceBegin + surfacePointerOffset;

    if (hasXoffsets) {
      int layerBound = std::min(intersectRect.getRight() - layerRect.getLeft(),
                                layerRect.getRight() - layerRect.getLeft()) %
                       layerRect.getWidth();
      drawCmykXoffset(currentSurfaceByte, bitmap, bitmapStart, bitmapOffset,
                      layerRect, intersectRect, layerBound, stride, layer);
    } else {
      drawCmyk(currentSurfaceByte, bitmap, bitmapStart, bitmapOffset, layer);
    }
  }

  int topLeftOffset = pointToOffset(toggledRect.getTopLeft(), stride);
  int bottomRightOffset =
      pointToOffset(toggledRect.getBottomRight(), stride) - stride;
  if (hasXoffsets) {
    int toggledBound = toggledRect.getRight() % stride;
    int toggledWidth = toggledRect.getWidth();
    convertCmykXoffset(surfaceBegin, targetBegin, topLeftOffset,
                       bottomRightOffset, toggledWidth, toggledBound, stride);
  } else {
    convertCmyk(surfaceBegin, targetBegin, topLeftOffset, bottomRightOffset);
  }

  cairo_surface_mark_dirty(surface->surface);

  if (!rgbCache.count(0))
    rgbCache[0] = surface;
}

void SliSource::clearBottomSurface() {
  if (toggled.none())
    return;

  if (toggled.all() && rgbCache.count(0)) {
    rgbCache[0]->clearSurface();
    // printf("Complete redraw! Area: %d pixels.\n",
    // getArea(rgbCache[0]->toRectangle()));
    return;
  }

  Scroom::Utils::Rectangle<int> spannedRect = spannedRectangle(toggled, layers);

  if (rgbCache.count(0)) {
    rgbCache[0]->clearSurface(spannedRect);
    // printf("Partial redraw! Area: %d pixels.\n", getArea(spannedRect));
  }
}
