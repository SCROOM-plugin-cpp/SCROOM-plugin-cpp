#include "slipresentation.hh"
#include "../sepsource.hh"
#include "slisource.hh"

#include "../varnish/varnish.hh"
#include "../varnish/varnish-helpers.hh"

#include <regex>
#include <boost/filesystem.hpp>

#include <scroom/cairo-helpers.hh>
#include <scroom/unused.hh>
    
SliPresentationInterface::WeakPtr SliPresentation::weakPtrToThis;

SliPresentation::SliPresentation(ScroomInterface::Ptr scroomInterface_): scroomInterface(scroomInterface_)
{
}

SliPresentation::Ptr SliPresentation::create(ScroomInterface::Ptr scroomInterface_)
{
  SliPresentation::Ptr result = Ptr(new SliPresentation(scroomInterface_));
  weakPtrToThis = result;
  
  // Can't do this in the constructor as it requires an existing shared pointer
  result->triggerRedrawFunc = boost::bind(&SliPresentation::triggerRedraw, result->shared_from_this<SliPresentation>());
  result->source = SliSource::create(result->triggerRedrawFunc);

  return result;
}

SliPresentation::~SliPresentation()
{
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
  source->computeHeightWidth();
  source->visible.resize(source->layers.size(), false);
  source->toggled.resize(source->layers.size(), true);
  
  transformationData = TransformationData::create();
  float xAspect = Xresolution / std::max(Xresolution, Yresolution);
  float yAspect = Yresolution / std::max(Xresolution, Yresolution);
  transformationData->setAspectRatio(1 / xAspect, 1 / yAspect);

  // Check if the aspect ratios align
  for (SliLayer::Ptr layer: source->layers)
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
        std::string varnishFile = std::string(*i++);
        printf("varnish_file: %s\n", varnishFile.c_str());
        fs::path imagePath = fs::path(dirPath) /= varnishFile;
        if (fs::exists(imagePath)) {
          printf("varnish file exists.\n");
          SliLayer::Ptr varnishLayer = SliLayer::create(imagePath.string(), varnishFile, 0, 0);
          fillVarnishOverlay(varnishLayer);
          varnish = Varnish::create(varnishLayer);
        } else {
          printf("[PANIC] varnish file not found: %s\n", imagePath.c_str());
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
          source->layers.push_back(layer);
        }
        else
        {
          SliLayer::Ptr layer = SliLayer::create(imagePath.string(), firstToken, xOffset, yOffset);
          fillFromTiff(layer);
          source->layers.push_back(layer);
        }
      }
      else
      {
        printf("Warning: Token '%s' in SLI file is not an existing file\n", firstToken.c_str());
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// SliPresentationInterface

void SliPresentation::wipeCache()
{
  source->wipeCache();
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
  return source->toggled;
}

boost::dynamic_bitset<> SliPresentation::getVisible()
{
  return source->visible;
}

void SliPresentation::setToggled(boost::dynamic_bitset<> bitmap)
{
  source->toggled = bitmap;
}

void SliPresentation::setVisible(boost::dynamic_bitset<> bitmap)
{
  source->visible = bitmap;
}

////////////////////////////////////////////////////////////////////////
// PresentationInterface

Scroom::Utils::Rectangle<double> SliPresentation::getRect()
{
  GdkRectangle rect;
  rect.x = 0;
  rect.y = 0;
  rect.width = source->total_width;
  rect.height = source->total_height;

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
  
  SurfaceWrapper::Ptr surfaceWrap = source->getSurface(zoom);

  // Check if it's not computed yet and we need to draw the waiting rectangle
  if (surfaceWrap == nullptr)
  {
    drawRectangle(cr, Color(0.5, 1, 0.5), pixelSize*(actualPresentationArea - presentationArea.getTopLeft()));
    return;
  }

  // The level that we need is in the cache, so draw it! 
  cairo_save(cr);
  cairo_translate(cr, -presentArea.x*pixelSize,-presentArea.y*pixelSize);
  if(zoom >= 0)
  {
    // We're using the bottom bitmap, hence we have to scale
    cairo_scale(cr, 1<<zoom, 1<<zoom);
    cairo_set_source_surface(cr, surfaceWrap->surface, 0, 0);
    cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
  }
  else
  {
    // Cached and reduced bitmap is already to scale
    cairo_set_source_surface(cr, surfaceWrap->surface, 0, 0);
  }
  cairo_paint(cr);
  cairo_restore(cr);

  /* --> Draw The varnish overlay if it exists */
  if (varnish) {
    varnish->drawOverlay(vi, cr, presentationArea, zoom);
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

void SliPresentation::viewAdded(ViewInterface::WeakPtr vi)
{
  controlPanel = SliControlPanel::create(vi, weakPtrToThis);

  // Provide the source with the means to enable and disable the widgets in the sidebar
  source->enableInteractions = boost::bind(&SliControlPanel::enableInteractions, controlPanel);
  source->disableInteractions = boost::bind(&SliControlPanel::disableInteractions, controlPanel);

  views.insert(vi);

  if (varnish)
  {
    varnish->setView(vi);
  }
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

