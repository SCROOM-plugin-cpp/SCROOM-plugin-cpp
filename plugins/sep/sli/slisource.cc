#include "slisource.hh"

#include <scroom/bitmap-helpers.hh>

SliSource::SliSource(boost::function<void()>& triggerRedrawFunc): triggerRedraw(triggerRedrawFunc)
{
  threadQueue = ThreadPool::Queue::createAsync();
  //triggerRedraw = triggerRedrawFunc;
}

SliSource::~SliSource()
{
}

SliSource::Ptr SliSource::create(boost::function<void()>& triggerRedrawFunc)
{
  return Ptr(new SliSource(triggerRedrawFunc));
}

/** 
 * Find the total height and width of the SLI file.
*/
void SliSource::computeHeightWidth()
{
  int max_xoffset = 0;
  int rightmost_l = 0;
  int max_yoffset = 0;
  int bottommost_l = 0;

  for (size_t i = 0; i < layers.size(); i++)
  {
    if (layers[i]->xoffset > max_xoffset)
    {
      rightmost_l = i;
      max_xoffset = layers[i]->xoffset;
    }

    if (layers[i]->yoffset > max_yoffset)
    {
      bottommost_l = i;
      max_yoffset = layers[i]->xoffset;
    }
  }
  
  total_width = layers[rightmost_l]->width + layers[rightmost_l]->xoffset;
  total_height = layers[bottommost_l]->height + layers[bottommost_l]->yoffset;
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
  const int sourceStride = rgbCache[zoom+1]->getStride();
  Scroom::Bitmap::SampleIterator<const uint8_t> sourceBase(rgbCache[zoom+1]->getBitmap(), 0, 8);

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
      auto sourceRow = sourceBase + 2*4*x;//2 pixels of 4 samples times x

      int sum_a = 0;
      int sum_r = 0;
      int sum_g = 0;
      int sum_b = 0;
      for (size_t row=0; row<2; row++, sourceRow += sourceStride)
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

    targetBase += targetStride; // Advance 1 row
    sourceBase += sourceStride * 2; // Advance 2 rows
  }

  // Make the cached bitmap available to the main thread
  rgbCache[zoom] = targetSurface;
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
  uint8_t *surface_begin = cairo_image_surface_get_data(surface->surface);
  uint32_t *target_begin = reinterpret_cast<uint32_t *>(surface_begin);
  uint8_t *current_surface_byte = surface_begin;

  // Rectangle (in bytes) of the toggled area
  Scroom::Utils::Rectangle<int> toggledRect;

  // TODO slightly duplicate code
  // Find the total rectangle spanned by all toggled layers
  size_t i = 0;
  for (; i < toggled.size(); i++)
  {
    if (toggled[i])
    {
      toggledRect = layers[i]->toBytesRectangle();
      i++;
      break;
    }
  }
  for (; i < toggled.size(); i++)
  {
    if (toggled[i])
      toggledRect = spannedRectangle(toggledRect, layers[i]->toBytesRectangle());
  }

  for (size_t j = 0; j < layers.size(); j++)
  {
    if (!visible[j])
      continue;

    auto layer = layers[j];
    auto bitmap = layer->bitmap;
    Scroom::Utils::Rectangle<int> layerRect = layer->toBytesRectangle();

    if (!layerRect.intersects(toggledRect))
      continue;

    // Rectangle area (in bytes) of the intersection between the toggled and current rectangles
    Scroom::Utils::Rectangle<int> intersectRect = toggledRect.intersection(layerRect);
    // index of the first pixel that needs to be drawn
    int bitmap_start = std::max(0, pointToOffset(layerRect, intersectRect.getTopLeft()));
    // offset of the last pixel from bitmap_start
    int bitmap_offset = intersectRect.getHeight() * layerRect.getWidth();
    // offset of the surface pointer from the top-left point of the surface
    int surface_pointer_offset = pointToOffset(intersectRect.getTopLeft(), stride);
    current_surface_byte = surface_begin + surface_pointer_offset;

    // TODO maybe do this 32 bits at a time and move calculations out of the loops
    int layerBound = std::min(intersectRect.getRight() - layerRect.getLeft(),
                              layerRect.getRight() - layerRect.getLeft()) %
                     layerRect.getWidth();
    for (int i = bitmap_start; i < bitmap_start + bitmap_offset;)
    {
      // increment the value of the current surface byte
      *current_surface_byte += std::min(bitmap[i], static_cast<uint8_t>(255 - *current_surface_byte));

      // go to the next surface byte
      current_surface_byte++;
      i++;

      // we are past the image bounds; go to the next next line
      if (i % layerRect.getWidth() == layerBound)
      {
        current_surface_byte += stride - intersectRect.getWidth();
        i += layerRect.getWidth() - intersectRect.getWidth();
      }
    }
  }

  double black;
  uint8_t C, M, Y, K, A, R, G, B;

  int topLeftOffset = pointToOffset(toggledRect.getTopLeft(), stride);
  int bottomRightOffset = pointToOffset(toggledRect.getBottomRight(), stride) - stride;
  int imageBound = toggledRect.getRight() % stride;
  for (int i = topLeftOffset; i < bottomRightOffset;)
  {
    C = surface_begin[i + 0];
    M = surface_begin[i + 1];
    Y = surface_begin[i + 2];
    K = surface_begin[i + 3];

    black = (1 - K / 255.0);
    A = 255;
    R = 255 * (1 - C / 255.0) * black;
    G = 255 * (1 - M / 255.0) * black;
    B = 255 * (1 - Y / 255.0) * black;

    target_begin[i / 4] = (A << 24) | (R << 16) | (G << 8) | B;
    i += 4;

    // we are past the image bounds; go to the next next line
    if (i % stride == imageBound)
    {
      i += stride - toggledRect.getWidth();
    }
  }

  cairo_surface_mark_dirty(surface->surface);
  toggled.reset();
  // Make the cached bitmap available to the main thread
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
    printf("Complete redraw!\n");
  }
  else
  {
    // TODO ask Luke how to improve this
    Scroom::Utils::Rectangle<int> toggledRect;
    size_t i = 0;
    for (; i < toggled.size(); i++)
    {
      if (toggled[i])
      {
        toggledRect = layers[i]->toRectangle();
        i++;
        break;
      }
    }
    for (; i < toggled.size(); i++)
    {
      if (toggled[i])
        toggledRect = spannedRectangle(toggledRect, layers[i]->toRectangle());
    }

    if (rgbCache.count(0))
    {
      rgbCache[0]->clearSurface(toggledRect);
      printf("Partial redraw! Area: %d\n", getArea(toggledRect));
    }
  }
}
