#include "slipresentation.hh"
#include "slicontrolpanel.hh"

#include <tiffio.h>
#include <string.h>

#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>
#include <scroom/unused.hh>

#define TIFFGetFieldChecked(file, field, ...) \
	if(1!=TIFFGetField(file, field, ##__VA_ARGS__)) \
	  throw std::invalid_argument("Field not present in tiff file: " #field);

SliPresentation::SliPresentation(ScroomInterface::Ptr scroomInterface_): scroomInterface(scroomInterface_)
{}

SliPresentation::Ptr SliPresentation::create(ScroomInterface::Ptr scroomInterface_)
{
  return Ptr(new SliPresentation(scroomInterface_));
}

SliPresentation::~SliPresentation()
{}

/**
 * Populate the SliLayer's bitmap with the bitmap data from the TiledBitmap
 */ 
void SliPresentation::extractBitmap(TiledBitmapInterface::Ptr tiledBitmap, SliLayer::Ptr sliLayer)
{
  Layer::Ptr bottomLayer = tiledBitmap->getBottomLayer();
  std::vector<uint8_t*>* sliBitmapData = sliLayer->getBitmap();

  int horTileCount = bottomLayer->getHorTileCount();
  int verTileCount = bottomLayer->getVerTileCount();
  bpp = bottomLayer->getTile(0,0)->bpp;

  for (int j = 0; j < verTileCount; j++)
  {
    for (int i = 0; i < horTileCount; i++)
    {
      CompressedTile::Ptr compressedTile = bottomLayer->getTile(j,i);
      ConstTile::Ptr constTile = compressedTile->getConstTileSync();
      auto tileData = constTile->data.get();
      int tileBitWidth = constTile->width * bpp;

      // Iterate over the rows of pixels in the tile
      for (int pixRow = 0; pixRow < constTile->height; pixRow++)
      {
        memcpy( &tileData + pixRow * tileBitWidth, 
                &sliBitmapData[j + pixRow] + i, 
                tileBitWidth);
      }
    }
  }

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
  parseSli(fileName); // TODO catch possible exceptions
  PresentationInterface::Ptr interf = scroomInterface->loadPresentation(layers[0]->getFilepath());
  return true;
}

////////////////////////////////////////////////////////////////////////
// PresentationInterface
////////////////////////////////////////////////////////////////////////

Scroom::Utils::Rectangle<double> SliPresentation::getRect()
{
  GdkRectangle rect;
  rect.x = 0;
  rect.y = -1;

  int width = 0;
  int height = 0;
  for (auto layer: layers)
  {
    width = std::max(layer->getWidth(), width);
    height = std::max(layer->getHeight(), height);
  }
  rect.width = width;
  rect.height = height;

  return rect;
}

void SliPresentation::viewAdded(ViewInterface::WeakPtr viewInterface)
{
  SliControlPanel::create(viewInterface);
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

void SliPresentation::redraw(ViewInterface::Ptr const &vi, cairo_t *cr,
                             Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  GdkRectangle presentArea = presentationArea.toGdkRectangle();
  UNUSED(vi);
  double pp = pixelSizeFromZoom(zoom);
  // double scale = pow(2, -zoom);

  Scroom::Utils::Rectangle<double> actualPresentationArea = getRect();
  drawOutOfBoundsWithBackground(cr, presentArea, actualPresentationArea, pp);

  SliLayer::Ptr image = layers[0];
  auto image_data = image->getBitmap();
  int height = image->getHeight();
  int width = image->getWidth();

  for (int y = 0; y < height; y++)
  {
    uint8_t *row = (*image_data)[y];
    for (int x = 0; x < width; x++)
    {
      // Convert CMYK to RGB
      double C = static_cast<double>(row[x*SPP+0]);
      double M = static_cast<double>(row[x*SPP+1]);
      double Y = static_cast<double>(row[x*SPP+2]);
      double K = static_cast<double>(row[x*SPP+3]);
      // printf("%f.%f.%f.%f\n", C, M, Y, K);

      double black = (1 - K/255);
      double R = (1 - C/255) * black;
      double G = (1 - M/255) * black;
      double B = (1 - Y/255) * black;

      // printf("%f.%f.%f\n",  R, G, B);

      cairo_rectangle(cr, -presentArea.x*pp+x*pp, -presentArea.y*pp+y*pp-pp, pp, pp);
      cairo_set_source_rgb(cr, R, G, B);
      cairo_fill(cr);
    }
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
 */
void SliPresentation::parseSli(const std::string &fileName)
{
  std::ifstream file(fileName);
  std::string str;
  std::string delimeter = ":";
  int line = 0;

  while (std::getline(file, str))
  { 
    if (str.empty()) continue;

    if (line == 0)
    {
      Xresolution = std::stoi(str.substr(str.find(delimeter)+1, std::string::npos));
    }
    else if (line == 1)
    {
      Yresolution = std::stoi(str.substr(str.find(delimeter)+1, std::string::npos));
    }
    else
    {
      std::string directory = trim(fileName.substr(0, fileName.find_last_of("/\\")+1));
      std::string filename = trim(str.substr(0,str.find(delimeter)));
      std::string name = filename.substr(0, filename.find("."));
      std::string filepath = directory + filename;
      int xoffset, yoffset;

      std::string temp = str.substr(str.find(delimeter)+1, std::string::npos);
      auto iss = std::istringstream{temp};
      auto token = std::string{};
      iss >> token;
      xoffset =  std::stoi(token);
      iss >> token;
      yoffset =  std::stoi(token);

      SliLayer::Ptr layer = SliLayer::create(filepath, name, xoffset, yoffset);
      layers.push_back(layer);
    }
    line++;
  }
}
