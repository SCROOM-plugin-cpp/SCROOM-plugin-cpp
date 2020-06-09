#include "slipresentation.hh"

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
  total_area = total_width*total_height;
  total_area_bytes = total_area * SPP;
}

/**
 * Create a file *.sli containing for example:
 * Xresolution: 900
 * Yresolution: 600
 * cmyktif1.tif : 0 0
 * cmyktif2.tif : 0 11
 */
bool SliPresentation::load(const std::string& fileName)
{
  parseSli(fileName);
  computeHeightWidth();
  CpuBound()->schedule(boost::bind(&SliPresentation::cacheBottomZoomLevelRgb, shared_from_this<SliPresentation>()),
                      PRIO_HIGHER, threadQueue);
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
        Xresolution = std::stoi(*i++);
      }
      else if (firstToken == "Yresolution:")
      {
        Yresolution = std::stoi(*i++);
      }
      else if (!firstToken.empty())
      {
        // Line seems to contain an image name
        fs::path imagePath = fs::path(dirPath) /= firstToken; 
        if (fs::exists(imagePath))
        {
          i++; // discard the colon
          int xOffset = std::stoi(*i++);
          int yOffset = std::stoi(*i++);
          layers.push_back(SliLayer::create(imagePath.string(), firstToken, xOffset, yOffset));
        }
        else
        {
          printf("Error: Cannot find file %s - skipping it\n", imagePath.string().c_str());
        }
      }
    }
  }
}

void SliPresentation::cacheZoomLevelRgb(int zoom)
{
  boost::recursive_mutex::scoped_lock lock(cachingPendingMtx);
  // Check if another thread has already computed the required bitmap in the meantime
  if (rgbCache.count(zoom))
  {
    return;
  }
  // We're basing the computation of the bitmap forzoom level x on the bitmap for zoom level
  // x-1. So ensure that the bitmap for zoom level x-1 is cached
  if (zoom < -1 && !rgbCache.count(zoom))
  {
    cacheZoomLevelRgb(zoom + 1);
  }
  printf("Computing for zoom %d\n", zoom);

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
  triggerRedraw();
}

void SliPresentation::cacheBottomZoomLevelRgb()
{
  boost::recursive_mutex::scoped_lock lock(cachingPendingMtx);
  // Check if another thread has already computed the required bitmap in the meantime
  if (rgbCache.count(0))
  {
    return;
  }

  SurfaceWrapper::Ptr surface = SurfaceWrapper::create(total_width, total_height, CAIRO_FORMAT_ARGB32);
  const int stride = surface->getStride();

  uint8_t* cur_surface_byte = surface->getBitmap();
  uint8_t* surface_begin = cur_surface_byte;
  uint32_t* target_begin = reinterpret_cast<uint32_t *>(surface_begin);

  for (auto layer : layers)
  {
    if (!layer->visible)
      continue;
    const auto bitmap = layer->bitmap;
    const int layer_height = layer->height;
    const int layer_width = layer->width;
    const int xoffset = layer->xoffset;
    const int yoffset = layer->yoffset;
    cur_surface_byte = surface_begin + stride*yoffset + xoffset*SPP;
    const int byte_width = layer_width*SPP;

    for (int i = 1; i <= layer_height*byte_width; i++)
    { 
      // we are past the image bounds; go to the next next line
      if (i % byte_width == 0)
      { 
        cur_surface_byte += stride - byte_width;
      }

      // increment the value of the current surface byte
      *cur_surface_byte += std::min(bitmap[i-1], static_cast<uint8_t>(255u - *cur_surface_byte));

      // go to the next surface byte
      cur_surface_byte++;
    }
  }

  for (int i = 0; i < stride*total_height; i+=SPP)
  {
    const uint8_t C = surface_begin[i+0];
    const uint8_t M = surface_begin[i+1];
    const uint8_t Y = surface_begin[i+2];
    const uint8_t K = surface_begin[i+3];

    const double black = (1 - (double)K/255);
    const uint8_t A = static_cast<uint8_t>(255);
    const uint8_t R = static_cast<uint8_t>(255 * (1 - (double)C/255) * black);
    const uint8_t G = static_cast<uint8_t>(255 * (1 - (double)M/255) * black);
    const uint8_t B = static_cast<uint8_t>(255 * (1 - (double)Y/255) * black);

    target_begin[i/SPP] = (A << 24) | (R << 16) | (G << 8) | B;
  }

  // Make the cached bitmap available to the main thread
  rgbCache[0] = surface;
  triggerRedraw();
}

////////////////////////////////////////////////////////////////////////
// SliPresentationInterface

void SliPresentation::wipeCache()
{
  rgbCache.clear();

  CpuBound()->schedule(boost::bind(&SliPresentation::cacheBottomZoomLevelRgb, shared_from_this<SliPresentation>()),
                      PRIO_HIGHER, threadQueue);
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

  cairo_save(cr);
  
  if(zoom > 0)
  {
    if (!rgbCache.count(0))
    {
      CpuBound()->schedule(boost::bind(&SliPresentation::cacheBottomZoomLevelRgb, shared_from_this<SliPresentation>()),
                      PRIO_HIGHER, threadQueue);
      drawRectangle(cr, Color(0.5, 1, 0.5), pixelSize*(actualPresentationArea - presentationArea.getTopLeft()));
    }
    else
    {
      // Let cairo do the zooming
      int multiplier = 1<<zoom;

      // We're using the bottom bitmap, hence we have to scale
      cairo_translate(cr, -presentArea.x*pixelSize,-presentArea.y*pixelSize);
      cairo_scale(cr, multiplier, multiplier);
      cairo_set_source_surface(cr, rgbCache[0]->surface, 0, 0);
      cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
      cairo_paint(cr);
    }
  }
  else
  {
    if (!rgbCache.count(zoom))
    {
      CpuBound()->schedule(boost::bind(&SliPresentation::cacheZoomLevelRgb, shared_from_this<SliPresentation>(), zoom),
                      PRIO_HIGHER, threadQueue);
      drawRectangle(cr, Color(0.5, 1, 0.5), pixelSize*(actualPresentationArea - presentationArea.getTopLeft()));
    }
    else
    {
      // Use our reduced bitmap for zooming
      cairo_translate(cr, -presentArea.x*pixelSize,-presentArea.y*pixelSize);

      // Cached bitmap is already to scale
      cairo_set_source_surface(cr, rgbCache[zoom]->surface, 0, 0);
      cairo_paint(cr);
    }
  }
  
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