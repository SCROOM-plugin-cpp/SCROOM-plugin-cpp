#include "slisource.hh"
#include "../sepsource.hh"

#include <scroom/bitmap-helpers.hh>

SliSource::SliSource(boost::function<void()> &triggerRedrawFunc) : triggerRedraw(triggerRedrawFunc)
{
  threadQueue = ThreadPool::Queue::create();
}

SliSource::~SliSource()
{
}

SliSource::Ptr SliSource::create(boost::function<void()> &triggerRedrawFunc)
{
  return Ptr(new SliSource(triggerRedrawFunc));
}

void SliSource::computeHeightWidth()
{
  auto rect = spannedRectangle(toggled, layers, true);
  total_width = rect.getWidth();
  total_height = rect.getHeight();
}

void SliSource::checkXoffsets()
{
  hasXoffsets = false;
  for (auto layer : layers)
  {
    if (layer->xoffset != 0)
      hasXoffsets = true;
  }
}

bool SliSource::addLayer(std::string imagePath, std::string filename, int xOffset, int yOffset)
{
  SliLayer::Ptr layer = SliLayer::create(imagePath, filename, xOffset, yOffset);
  auto extension = filename.substr(filename.find_last_of("."));
  boost::to_lower(extension);
  
  if (extension == ".sep")
  {
    SepSource::fillSliLayer(layer);
  }
  else if (extension == ".tif" || extension == ".tiff")
  {
    if (!fillFromTiff(layer))
    {
      return false;
    }
  }
  else
  {
    printf("Error: File extension of %s is not supported\n", filename.c_str());
    return false;
  }
  layers.push_back(layer);
  return true;
}

void SliSource::wipeCache()
{
  // Erase all cache levels except for the bottom layer
  auto it = rgbCache.begin();
  while (it != rgbCache.end())
  {
    if (it->first != 0)
    {
      it = rgbCache.erase(it);
    }
    else
    {
      it++;
    }
  }
  // Clear the area of the last toggled layer from the bottom surface
  clearBottomSurface();
}

SurfaceWrapper::Ptr SliSource::getSurface(int zoom)
{
  if (!rgbCache.count(std::min(0, zoom)) || rgbCache[0]->clear)
  {
    CpuBound()->schedule(boost::bind(&SliSource::fillCache, shared_from_this<SliSource>(), zoom),
                         PRIO_HIGHER, threadQueue);
    return nullptr;
  }
  else
  {
    return rgbCache[std::min(0, zoom)];
  }
}
void SliSource::fillCache(int zoom)
{
  mtx.lock();
  disableInteractions();

  if (!rgbCache.count(0) || rgbCache[0]->clear)
  {
    computeRgb();
  }
  if (zoom < 0)
  {
    for (int i = -1; i >= zoom; i--)
    {
      if (!rgbCache.count(i))
      {
        reduceRgb(i);
      }
    }
  }
  enableInteractions();
  mtx.unlock();
  triggerRedraw();
}

void SliSource::reduceRgb(int zoom)
{
  // printf("Computing for zoom %d\n", zoom);

  const int sourceWidth = total_width / pow(2, -zoom - 1);
  const int sourceHeight = total_height / pow(2, -zoom - 1);
  const int sourceStride = rgbCache[zoom + 1]->getStride();
  Scroom::Bitmap::SampleIterator<const uint8_t> sourceBase(rgbCache[zoom + 1]->getBitmap(), 0, 8);

  const int targetWidth = sourceWidth / 2;
  const int targetHeight = sourceHeight / 2;
  SurfaceWrapper::Ptr targetSurface = SurfaceWrapper::create(targetWidth, targetHeight, CAIRO_FORMAT_ARGB32);
  const int targetStride = targetSurface->getStride();
  Scroom::Bitmap::SampleIterator<uint8_t> targetBase(targetSurface->getBitmap(), 0, 8);

  for (int y = 0; y < targetHeight; y++)
  {
    auto targetSample = targetBase;

    for (int x = 0; x < targetWidth; x++)
    {
      // We want to store the average colour of the 2*2 pixel image
      // with (x, y) as its top-left corner into targetSample.
      auto sourceRow = sourceBase + 2 * 4 * x; //2 pixels of 4 samples times x

      int sum_a = 0;
      int sum_r = 0;
      int sum_g = 0;
      int sum_b = 0;
      for (size_t row = 0; row < 2; row++, sourceRow += sourceStride)
      {
        auto sourceSample = sourceRow;
        for (size_t current = 0; current < 2; current++)
        {
          sum_a += *sourceSample++;
          sum_r += *sourceSample++;
          sum_g += *sourceSample++;
          sum_b += *sourceSample++;
        }
      }

      (targetSample++).set(sum_a / 4);
      (targetSample++).set(sum_r / 4);
      (targetSample++).set(sum_g / 4);
      (targetSample++).set(sum_b / 4);
    }

    targetBase += targetStride;     // Advance 1 row
    sourceBase += sourceStride * 2; // Advance 2 rows
  }

  // Make the cached bitmap available to the main thread
  rgbCache[zoom] = targetSurface;
}

void SliSource::convertCmykXoffset(uint8_t *surfacePointer, uint32_t *targetPointer, int topLeftOffset, int bottomRightOffset, int toggledWidth, int toggledBound, int stride)
{
  double black;
  uint8_t C, M, Y, K, A, R, G, B;

  for (int i = topLeftOffset; i < bottomRightOffset;)
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
    i += 4; // SPP = 4

    // we are past the image bounds; go to the next next line
    if (i % stride == toggledBound)
    {
      i += stride - toggledWidth;
    }
  }
}

void SliSource::convertCmyk(uint8_t *surfacePointer, uint32_t *targetPointer, int topLeftOffset, int bottomRightOffset)
{
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

void SliSource::drawCmyk(uint8_t *surfacePointer, uint8_t *bitmap, int bitmapStart, int bitmapOffset)
{
  for (int i = bitmapStart; i < bitmapStart + bitmapOffset; i++)
  {
    // increment the value of the current surface byte
    *surfacePointer += std::min(bitmap[i], static_cast<uint8_t>(255 - *surfacePointer));

    // go to the next surface byte
    surfacePointer++;
  }
}

void SliSource::drawCmykXoffset(uint8_t *surfacePointer, uint8_t *bitmap, int bitmapStart, int bitmapOffset, Scroom::Utils::Rectangle<int> layerRect, Scroom::Utils::Rectangle<int> intersectRect, int layerBound, int stride)
{
  for (int i = bitmapStart; i < bitmapStart + bitmapOffset;)
  {
    // increment the value of the current surface byte
    *surfacePointer += std::min(bitmap[i], static_cast<uint8_t>(255 - *surfacePointer));

    // go to the next surface byte
    surfacePointer++;
    i++;

    // we are past the image bounds; go to the next next line
    if (i % layerRect.getWidth() == layerBound)
    {
      surfacePointer += stride - intersectRect.getWidth();
      i += layerRect.getWidth() - intersectRect.getWidth();
    }
  }
}

void SliSource::computeRgb()
{
  // Update the visibility of all layers according to the toggled ones
  visible ^= toggled;

  SurfaceWrapper::Ptr surface = SurfaceWrapper::create();

  // Check if cache surface exists first
  if (rgbCache.count(0))
  {
    surface = rgbCache[0];
  }
  else
  {
    surface = SurfaceWrapper::create(total_width, total_height, CAIRO_FORMAT_ARGB32);
  }
  const int stride = surface->getStride();
  cairo_surface_flush(surface->surface);
  uint8_t *surfaceBegin = cairo_image_surface_get_data(surface->surface);
  uint32_t *targetBegin = reinterpret_cast<uint32_t *>(surfaceBegin);
  uint8_t *currentSurfaceByte = surfaceBegin;

  // Rectangle (in bytes) of the toggled area
  Scroom::Utils::Rectangle<int> toggledRect = toBytesRectangle(spannedRectangle(toggled, layers));

  for (size_t j = 0; j < layers.size(); j++)
  {
    if (!visible[j])
      continue;

    auto layer = layers[j];
    auto bitmap = layer->bitmap;
    Scroom::Utils::Rectangle<int> layerRect = toBytesRectangle(layer->toRectangle());

    if (!layerRect.intersects(toggledRect))
      continue;

    // Rectangle area (in bytes) of the intersection between the toggled and current rectangles
    Scroom::Utils::Rectangle<int> intersectRect = toggledRect.intersection(layerRect);
    // index of the first pixel that needs to be drawn
    int bitmapStart = pointToOffset(layerRect, intersectRect.getTopLeft());
    // offset of the last pixel from bitmapStart
    int bitmapOffset = intersectRect.getHeight() * layerRect.getWidth();
    // offset of the surface pointer from the top-left point of the surface
    int surfacePointerOffset = pointToOffset(intersectRect.getTopLeft(), stride);
    currentSurfaceByte = surfaceBegin + surfacePointerOffset;

    if (hasXoffsets)
    {
      int layerBound = std::min(intersectRect.getRight() - layerRect.getLeft(),
                                layerRect.getRight() - layerRect.getLeft()) %
                       layerRect.getWidth();
      drawCmykXoffset(currentSurfaceByte, bitmap, bitmapStart, bitmapOffset, layerRect, intersectRect, layerBound, stride);
    }
    else
    {
      drawCmyk(currentSurfaceByte, bitmap, bitmapStart, bitmapOffset);
    }
  }

  int topLeftOffset = pointToOffset(toggledRect.getTopLeft(), stride);
  int bottomRightOffset = pointToOffset(toggledRect.getBottomRight(), stride) - stride;
  if (hasXoffsets)
  {
    int toggledBound = toggledRect.getRight() % stride;
    int toggledWidth = toggledRect.getWidth();
    convertCmykXoffset(surfaceBegin, targetBegin, topLeftOffset, bottomRightOffset, toggledWidth, toggledBound, stride);
  }
  else
  {
    convertCmyk(surfaceBegin, targetBegin, topLeftOffset, bottomRightOffset);
  }

  cairo_surface_mark_dirty(surface->surface);
  toggled.reset();
  if (!rgbCache.count(0))
    rgbCache[0] = surface;
  rgbCache[0]->clear = false;
}

void SliSource::clearBottomSurface()
{
  if (toggled.none())
    return;

  if (toggled.all() && rgbCache.count(0))
  {
    rgbCache[0]->clearSurface();
    printf("Complete redraw! Area: %d pixels.\n", getArea(rgbCache[0]->toRectangle()));
    return;
  }

  Scroom::Utils::Rectangle<int> spannedRect = spannedRectangle(toggled, layers);

  if (rgbCache.count(0))
  {
    rgbCache[0]->clearSurface(spannedRect);
    printf("Partial redraw! Area: %d pixels.\n", getArea(spannedRect));
  }
}