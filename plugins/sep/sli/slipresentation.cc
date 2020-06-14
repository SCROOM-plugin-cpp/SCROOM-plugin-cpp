#include "slipresentation.hh"
#include "../sepsource.hh"

#include <regex>
#include <boost/filesystem.hpp>

#include <scroom/cairo-helpers.hh>
#include <scroom/unused.hh>
#include <scroom/bitmap-helpers.hh>
    
SliPresentationInterface::WeakPtr SliPresentation::weakPtrToThis;

SliPresentation::SliPresentation(ScroomInterface::Ptr scroomInterface_): scroomInterface(scroomInterface_)
{
  threadQueue = ThreadPool::Queue::createAsync();
}

SliPresentation::Ptr SliPresentation::create(ScroomInterface::Ptr scroomInterface_)
{
  SliPresentation::Ptr result = Ptr(new SliPresentation(scroomInterface_));
  weakPtrToThis = result;
  return result;
}

SliPresentation::~SliPresentation()
{
}

/** 
 * Find the total height and width of the SLI file.
*/
void SliPresentation::computeHeightWidth()
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

/**
 * Create a file *.sli containing for example:
 * Xresolution: 900
 * Yresolution: 600
 * varnish_file: varnish.tif
 * cmyktif1.tif : 0 0
 * cmyktif2.tif : 0 11
 */
bool SliPresentation::load(const std::string& fileName)
{
  parseSli(fileName);
  computeHeightWidth();
  visible.resize(layers.size(), false);
  toggled.resize(layers.size(), true);
  
  transformationData = TransformationData::create();
  float xAspect = Xresolution / std::max(Xresolution, Yresolution);
  float yAspect = Yresolution / std::max(Xresolution, Yresolution);
  transformationData->setAspectRatio(1 / xAspect, 1 / yAspect);

  // Check if the aspect ratios align
  for (SliLayer::Ptr layer: layers)
  {
    if (std::abs((layer->xAspect / layer->yAspect) - (xAspect / yAspect)) > 1e-3)
    {
      printf("Warning: Aspect ratio mismatch - SLI file defines xAspect=%.3f and yAspect=%.3f "
             "but layer %s has xAspect=%.3f and yAspect=%.3f\n", 
             xAspect, yAspect, layer->name.c_str(), layer->xAspect, layer->yAspect);
    }
  }
  return true;
}

void SliPresentation::parseSli(const std::string &sliFileName)
{
  std::ifstream file(sliFileName);
  std::string line;
  std::regex e("\\s+"); // split on whitespaces
  std::sregex_token_iterator j;
  namespace fs = boost::filesystem;
  std::string dirPath = fs::path(sliFileName).parent_path().string();

  // Iterate over the lines
  while (std::getline(file, line))  
  {
    std::sregex_token_iterator i(line.begin(), line.end(), e, -1);
    // Iterate over the whitespace-separated tokens of the line
    while(i != j)
    {
      std::string firstToken = *i++;
      if (firstToken == "Xresolution:")
      {
        Xresolution = std::stof(*i++);
        printf("xresolution: %f\n", Xresolution);
      }
      else if (firstToken == "Yresolution:")
      {
        Yresolution = std::stof(*i++);
        printf("yresolution: %f\n", Yresolution);
      }
      else if (firstToken == "varnish_file:")
      {
        varnishFile = std::string(*i++);
        printf("varnish_file: %s\n", varnishFile.c_str());
        if (fs::exists(fs::path(dirPath) /= varnishFile)) {
          printf("varnish file exists.\n");
        }
      }
      else if (fs::exists(fs::path(dirPath) /= firstToken))
      {
        // Line contains name of an existing file
        fs::path imagePath = fs::path(dirPath) /= firstToken;
        i++; // discard the colon
        int xOffset = std::stoi(*i++);
        int yOffset = std::stoi(*i++);
        if (fs::extension(firstToken) == ".sep")
        {
          SliLayer::Ptr layer = SliLayer::create(imagePath.string(), firstToken, xOffset, yOffset);
          SepSource::fillSliLayer(layer);
          layers.push_back(layer);
        }
        else if (fs::extension(firstToken) == ".tif")
        {
          SliLayer::Ptr layer = SliLayer::create(imagePath.string(), firstToken, xOffset, yOffset);
          fillFromTiff(layer);
          layers.push_back(layer);
        }
        else
        {
          printf("Warning: File extension of %s not supported - skipping file\n", firstToken.c_str());
        }
      }
      else
      {
        printf("Warning: Token '%s' in SLI file is not an exiting file\n", firstToken.c_str());
      }
    }
  }
}

void SliPresentation::clearBottomSurface()
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

void SliPresentation::fillCache(int zoom)
{
  mtx.lock();
  controlPanel->disableInteractions();

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
  controlPanel->enableInteractions();
  mtx.unlock();
  triggerRedraw();
}

void SliPresentation::reduceRgb(int zoom)
{
  // printf("Computing for zoom %d\n", zoom);

  const int sourceWidth = total_width / pow(2, -zoom - 1);
  const int sourceHeight = total_height / pow(2, -zoom - 1);
  const int sourceStride = rgbCache[zoom+1]->getStride();
  Scroom::Bitmap::SampleIterator<const uint8_t> sourceBase(rgbCache[zoom+1]->getBitmap(), 0, 8);
  const unsigned int sourceMax = sourceBase.pixelMask;

  const int targetWidth = sourceWidth / 2;
  const int targetHeight = sourceHeight / 2;
  SurfaceWrapper::Ptr targetSurface = SurfaceWrapper::create(targetWidth, targetHeight, CAIRO_FORMAT_ARGB32);
  const int targetStride = targetSurface->getStride();
  Scroom::Bitmap::SampleIterator<uint8_t> targetBase(targetSurface->getBitmap(), 0, 8);
  const unsigned int targetMax = targetBase.pixelMask;

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

      (targetSample++).set(sum_a * targetMax / sourceMax / 4);
      (targetSample++).set(sum_r * targetMax / sourceMax / 4);
      (targetSample++).set(sum_g * targetMax / sourceMax / 4);
      (targetSample++).set(sum_b * targetMax / sourceMax / 4);
    }

    targetBase += targetStride; // Advance 1 row
    sourceBase += sourceStride * 2; // Advance 2 rows
  }

  // Make the cached bitmap available to the main thread
  rgbCache[zoom] = targetSurface;
}

void SliPresentation::computeRgb()
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

TransformationData::Ptr SliPresentation::getTransformationData() const
{
  return transformationData;
}

////////////////////////////////////////////////////////////////////////
// SliPresentationInterface

void SliPresentation::wipeCache()
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

void SliPresentation::triggerRedraw()
{
  for (ViewInterface::WeakPtr view: views)
  {
    ViewInterface::Ptr viewPtr = view.lock();
    gdk_threads_enter();
    viewPtr->invalidate();
    gdk_threads_leave();
  }
}

boost::dynamic_bitset<> SliPresentation::getToggled()
{
  return toggled;
}

boost::dynamic_bitset<> SliPresentation::getVisible()
{
  return visible;
}

void SliPresentation::setToggled(boost::dynamic_bitset<> bitmap)
{
  toggled = bitmap;
}

void SliPresentation::setVisible(boost::dynamic_bitset<> bitmap)
{
  visible = bitmap;
}

////////////////////////////////////////////////////////////////////////
// PresentationInterface

Scroom::Utils::Rectangle<double> SliPresentation::getRect()
{
  GdkRectangle rect;
  rect.x = 0;
  rect.y = 0;
  rect.width = total_width;
  rect.height = total_height;

  return rect;
}

void SliPresentation::redraw(ViewInterface::Ptr const &vi, cairo_t *cr,
                             Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  UNUSED(vi);
  GdkRectangle presentArea = presentationArea.toGdkRectangle();
  Scroom::Utils::Rectangle<double> actualPresentationArea = getRect();
  double pixelSize = pixelSizeFromZoom(zoom);
 
  drawOutOfBoundsWithBackground(cr, presentArea, actualPresentationArea, pixelSize);
  
  if (!rgbCache.count(std::min(0, zoom)) || rgbCache[0]->clear)
  {
    drawRectangle(cr, Color(0.5, 1, 0.5), pixelSize*(actualPresentationArea - presentationArea.getTopLeft()));
    CpuBound()->schedule(boost::bind(&SliPresentation::fillCache, shared_from_this<SliPresentation>(), zoom),
                      PRIO_HIGHER, threadQueue);
    return;
  }

  // The level that we need is in the cache, so draw it! 
  cairo_save(cr);
  cairo_translate(cr, -presentArea.x*pixelSize,-presentArea.y*pixelSize);
  if(zoom >= 0)
  {
    // We're using the bottom bitmap, hence we have to scale
    cairo_scale(cr, 1<<zoom, 1<<zoom);
    cairo_set_source_surface(cr, rgbCache[0]->surface, 0, 0);
    cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
  }
  else
  {
    // Cached and reduced bitmap is already to scale
    cairo_set_source_surface(cr, rgbCache[zoom]->surface, 0, 0);
  }
  cairo_paint(cr);
  cairo_restore(cr);
}

bool SliPresentation::getProperty(const std::string& name, std::string& value)
{
  std::map<std::string, std::string>::iterator p = properties.find(name);
  bool found = false;
  if (p == properties.end())
  {
    found = false;
    value = "";
  }
  else
  {
    found = true;
    value = p->second;
  }

  return found;
}

bool SliPresentation::isPropertyDefined(const std::string& name)
{
  return properties.end() != properties.find(name);
}

std::string SliPresentation::getTitle()
{
  return "slipresentation";
}

////////////////////////////////////////////////////////////////////////
// PresentationBase

void SliPresentation::viewAdded(ViewInterface::WeakPtr viewInterface)
{
  controlPanel = SliControlPanel::create(viewInterface, weakPtrToThis);
  views.insert(viewInterface);
}

void SliPresentation::viewRemoved(ViewInterface::WeakPtr vi)
{
  views.erase(vi);
}

std::set<ViewInterface::WeakPtr> SliPresentation::getViews()
{
  return views;
}


// ////////////////////////////////////////////////////////////////////////
// // PipetteViewInterface
// ////////////////////////////////////////////////////////////////////////

// // void SliPresentation::getPixelAverages(Scroom::Utils::Rectangle<int> area)
// // {

// // }

