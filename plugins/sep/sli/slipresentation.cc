#include "slipresentation.hh"
#include "../seppresentation.hh"

#include <string.h>

#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>
#include <scroom/unused.hh>
#include <scroom/bitmap-helpers.hh>


#define TIFFGetFieldChecked(file, field, ...) \
	if(1!=TIFFGetField(file, field, ##__VA_ARGS__)) \
	  throw std::invalid_argument("Field not present in tiff file: " #field);
    
SliPresentationInterface::WeakPtr SliPresentation::weakPtrToThis;

SliPresentation::SliPresentation(ScroomInterface::Ptr scroomInterface_): scroomInterface(scroomInterface_)
{}

SliPresentation::Ptr SliPresentation::create(ScroomInterface::Ptr scroomInterface_)
{
  SliPresentation::Ptr result = Ptr(new SliPresentation(scroomInterface_));
  weakPtrToThis = result;
  return result;
}

SliPresentation::~SliPresentation()
{
  std::map<int, uint8_t*>::iterator it;

  // Free all the memory that contains cached bitmaps
  for ( it = rgbCache.begin(); it != rgbCache.end(); it++)
  {
    free(it->second);
  }
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
    if (layers[i]->getXoffset() > max_xoffset)
    {
      rightmost_l = i;
      max_xoffset = layers[i]->getXoffset();
    }

    if (layers[i]->getYoffset() > max_yoffset)
    {
      bottommost_l = i;
      max_yoffset = layers[i]->getXoffset();
    }
  }
  
  total_width = layers[rightmost_l]->getWidth() + layers[rightmost_l]->getXoffset();
  total_height = layers[bottommost_l]->getHeight() + layers[bottommost_l]->getYoffset();
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
  cacheBottomZoomLevelRgb();
  PresentationInterface::Ptr interf = scroomInterface->loadPresentation(layers[0]->getFilepath());
  cacheBottomZoomLevelRgb();
  return true;
}

/* Trimming functions */
// TODO maybe move these into a utils file or something
const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s)
{
  return rtrim(ltrim(s));
}

/**
 * Parses the content of an SLI file and stores it as a vector of SliLayer
 * TODO throw possible exceptions
 */
void SliPresentation::parseSli(const std::string &fileName)
{
  std::ifstream file(fileName);
  std::string str;
  int line = 0;

  while (std::getline(file, str))
  { 
    if (str.empty()) continue;

    // Remove :
    str.erase(std::remove(str.begin(), str.end(), ':'), str.end());

    if (line == 0)
    {
      Xresolution = std::stoi(str.substr(str.find(" ")+1, std::string::npos));
    }
    else if (line == 1)
    {
      Yresolution = std::stoi(str.substr(str.find(" ")+1, std::string::npos));
    }
    else
    {
      std::string directory = trim(fileName.substr(0, fileName.find_last_of("/\\")+1));
      std::tuple<std::string, int, int> data {"Layer", 0, 0};

      std::vector<std::string> tokens;
      auto iss = std::istringstream{str};
      auto token = std::string{};
      while (iss >> token) {
        tokens.push_back(trim(token));
      }

      if (tokens.size() > 0)
        std::get<0>(data) = tokens[0];
      if (tokens.size() > 1)
        std::get<1>(data) = std::stoi(tokens[1]);
      if (tokens.size() > 2)
        std::get<2>(data) = std::stoi(tokens[2]);

      std::string name = std::get<0>(data).substr(0, std::get<0>(data).find("."));
      std::string filepath = directory + std::get<0>(data);

      SliLayer::Ptr layer = SliLayer::create(filepath, name, std::get<1>(data), std::get<2>(data));

      // Getting the SliLayer filled by the SEP plugin, when it's done
      //SliLayer::Ptr layer = SliLayer::create(filepath, name, xoffset, yoffset);
      //SepPresentation::Ptr sepPresentation = SepPresentation::create();
      //sepPresentation->fillSliLayer(layer);

      layers.push_back(layer);
    }
    line++;
  }
}

////////////////////////////////////////////////////////////////////////
// SliPresentationInterface

void SliPresentation::wipeCache()
{
  std::map<int, uint8_t*>::iterator it;

  // Free all the memory that contains cached bitmaps
  for ( it = rgbCache.begin(); it != rgbCache.end(); it++)
  {
    free(it->second);
  }
  rgbCache.clear();
}

void SliPresentation::triggerRedraw()
{
  for (ViewInterface::WeakPtr view: views)
  {
    ViewInterface::Ptr viewPtr = view.lock();
    viewPtr->invalidate();
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

void SliPresentation::cacheZoomLevelRgb(int zoom)
{
  // We're basing the computation of the bitmap forzoom level x on the bitmap for zoom level
  // x-1. So ensure that the bitmap for zoom level x-1 is cached
  if (zoom < -1 && !rgbCache.count(zoom))
  {
    cacheZoomLevelRgb(zoom + 1);
  }
  printf("Computing for zoom %d\n", zoom);

  const int sourceWidth = total_width / pow(2, -zoom - 1);
  const int sourceHeight = total_height / pow(2, -zoom - 1);
  const int sourceStride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, sourceWidth);
  Scroom::Bitmap::SampleIterator<const uint8_t> sourceBase(rgbCache[zoom+1], 0, 8);
  const unsigned int sourceMax = sourceBase.pixelMask;

  const int targetWidth = sourceWidth / 2;
  const int targetHeight = sourceHeight / 2;
  const int targetStride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, targetWidth);
  uint8_t* targetBitmap = static_cast<uint8_t*>(calloc(targetHeight * targetStride, 1));
  rgbCache[zoom] = targetBitmap;
  Scroom::Bitmap::SampleIterator<uint8_t> targetBase(targetBitmap, 0, 8);
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
  
}

void SliPresentation::cacheBottomZoomLevelRgb()
{
  rgbCache[0] = static_cast<uint8_t*>(calloc(total_area, SPP));

  const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, total_width);
  cairo_surface_t *surface = cairo_image_surface_create_for_data(rgbCache[0], CAIRO_FORMAT_ARGB32, total_width, total_height, stride);

  uint8_t* cur_surface_byte = reinterpret_cast<uint8_t*>(cairo_image_surface_get_data(surface));
  uint8_t* surface_begin = cur_surface_byte;
  uint32_t* target_begin = reinterpret_cast<uint32_t *>(surface_begin);

  for (auto layer : layers)
  {
    if (!layer->visible)
      continue;
    const auto bitmap = layer->getBitmap();
    const int layer_height = layer->getHeight();
    const int layer_width = layer->getWidth();
    const int xoffset = layer->getXoffset();
    const int yoffset = layer->getYoffset();
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

}

void SliPresentation::redraw(ViewInterface::Ptr const &vi, cairo_t *cr,
                             Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  GdkRectangle presentArea = presentationArea.toGdkRectangle();
  UNUSED(vi);
  double pixelSize = pixelSizeFromZoom(zoom);
  cairo_surface_t *surface;

  Scroom::Utils::Rectangle<double> actualPresentationArea = getRect();
  drawOutOfBoundsWithBackground(cr, presentArea, actualPresentationArea, pixelSize);

  // TODO why do very large images crash when fully zoomed out and 
  // why does Scroom/Cairo already show the visible area only?

  // If the bottom zoom level is not cached yet, cache it
  if (!rgbCache.count(0))
  {
    cacheBottomZoomLevelRgb();
  }
  // If we're zoomed out and the zoom layer is not cached yet, cache it
  if (zoom < 0 && !rgbCache.count(zoom))
  {
    cacheZoomLevelRgb(zoom);
  }

  cairo_save(cr);
  cairo_translate(cr, -presentArea.x*pixelSize,-presentArea.y*pixelSize);

  if(zoom > 0)
  {
    // Let cairo do the zooming
    const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, total_width);
    int multiplier = 1<<zoom;
    surface = cairo_image_surface_create_for_data(rgbCache[0], CAIRO_FORMAT_ARGB32, total_width, total_height, stride);

    // We're using the bottom bitmap, hence we have to scale
    cairo_scale(cr, multiplier, multiplier);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
  }
  else
  {
    // Use our reduced bitmap for zooming
    const int zoomLevelWidth = total_width / pow(2, -zoom);
    const int zoomLevelHeight = total_height / pow(2, -zoom);
    const int zoomLevelstride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, zoomLevelWidth);
    surface = cairo_image_surface_create_for_data(rgbCache[zoom], CAIRO_FORMAT_ARGB32, zoomLevelWidth, zoomLevelHeight, zoomLevelstride);

    // Cached bitmap is already to scale
    cairo_set_source_surface(cr, surface, 0, 0);
  }

  cairo_paint(cr);
  cairo_restore(cr);
  cairo_surface_destroy(surface);

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