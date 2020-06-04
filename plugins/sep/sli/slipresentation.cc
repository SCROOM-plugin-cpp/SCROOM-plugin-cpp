#include "slipresentation.hh"
#include "../seppresentation.hh"

#include <string.h>

#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>
#include <scroom/unused.hh>


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
{}

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
 
  /** 
   * Find the total size of the SLI file.
   * Assumes the the layer with the largest xoffset and the last layer
   * are the bounds of the image.
  */
  int rightmost = 0;
  for (size_t i = 0; i < layers.size(); i++)
  {
    if (layers[i]->getXoffset() > rightmost)
      rightmost = i;
  }
  total_width = layers[rightmost]->getWidth() + layers[rightmost]->getXoffset();
  int last_layer = layers.size()-1;
  total_height = layers[last_layer]->getHeight() + layers[last_layer]->getYoffset();

  PresentationInterface::Ptr interf = scroomInterface->loadPresentation(layers[0]->getFilepath());
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

void SliPresentation::toggleLayer(int index)
{

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
  GdkRectangle presentArea = presentationArea.toGdkRectangle();
  UNUSED(vi);
  double pixelSize = pixelSizeFromZoom(zoom);
  double scale = pow(2, zoom);

  Scroom::Utils::Rectangle<double> actualPresentationArea = getRect();
  drawOutOfBoundsWithBackground(cr, presentArea, actualPresentationArea, pixelSize);

  for (auto layer : layers)
  {
    auto bitmap = layer->getBitmap();
    int layer_height = layer->getHeight();
    int layer_width = layer->getWidth();
    int xoffset = layer->getXoffset();
    int yoffset = layer->getYoffset();

    // https://stackoverflow.com/q/43127823
    cairo_surface_t *surface;
    unsigned char *cur_surface_row;
    int stride;
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, total_width, total_height);
    cur_surface_row = cairo_image_surface_get_data(surface);
    stride = cairo_image_surface_get_stride(surface);

    cairo_surface_flush(surface);
    for (int y = 0; y < layer_height; y++, cur_surface_row+=stride)
    {
      uint8_t *bitmap_row = bitmap + y*stride;
      uint32_t *surface_row = reinterpret_cast<uint32_t*>(cur_surface_row);
      for (int x = 0; x < layer_width; x++)
      {
        // Convert CMYK to RGB
        uint8_t C = bitmap_row[x*SPP+0];
        uint8_t M = bitmap_row[x*SPP+1];
        uint8_t Y = bitmap_row[x*SPP+2];
        uint8_t K = bitmap_row[x*SPP+3];

        double black = (1 - (double)K/255);
        uint8_t R = static_cast<uint8_t>(255 * (1 - (double)C/255) * black);
        uint8_t G = static_cast<uint8_t>(255 * (1 - (double)M/255) * black);
        uint8_t B = static_cast<uint8_t>(255 * (1 - (double)Y/255) * black);

        surface_row[x] = (255u << 24) | (R << 16) | (G << 8) | B;
      }
    }
  // https://stackoverflow.com/q/7145780
  cairo_save(cr);
  cairo_surface_mark_dirty(surface);
  cairo_pattern_t* pattern = cairo_pattern_create_for_surface(surface);
  cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
  cairo_translate(cr, -presentArea.x*pixelSize+xoffset*pixelSize,-presentArea.y*pixelSize+yoffset*pixelSize);
  cairo_scale(cr, scale, scale);
  cairo_set_source(cr, pattern);
  cairo_paint(cr);
  cairo_restore(cr);

  cairo_surface_destroy(surface);
  cairo_pattern_destroy(pattern);
  }
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
