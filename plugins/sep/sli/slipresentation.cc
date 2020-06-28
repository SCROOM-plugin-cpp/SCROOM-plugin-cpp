#include "slipresentation.hh"
#include "../sep-helpers.hh"
#include "slisource.hh"

#include "../varnish/varnish.hh"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <regex>

#include <scroom/cairo-helpers.hh>
#include <scroom/unused.hh>

SliPresentationInterface::WeakPtr SliPresentation::weakPtrToThis;

SliPresentation::SliPresentation(ScroomInterface::Ptr scroomInterface_)
    : scroomInterface(scroomInterface_) {}

SliPresentation::Ptr
SliPresentation::create(ScroomInterface::Ptr scroomInterface_) {
  SliPresentation::Ptr result = Ptr(new SliPresentation(scroomInterface_));
  weakPtrToThis = result;

  // Can't do this in the constructor as it requires an existing shared pointer
  result->triggerRedrawFunc =
      boost::bind(&SliPresentation::triggerRedraw,
                  result->shared_from_this<SliPresentation>());
  result->source = SliSource::create(result->triggerRedrawFunc);

  return result;
}

SliPresentation::~SliPresentation() {}

bool SliPresentation::load(const std::string &fileName) {
  filepath = fileName;
  if (!parseSli(fileName)) {
    return false;
  }
  properties[PIPETTE_PROPERTY_NAME] = "";
  source->visible.resize(source->layers.size(), false);
  source->toggled.resize(source->layers.size(), true);
  source->computeHeightWidth();
  source->checkXoffsets();

  transformationData = TransformationData::create();
  float xAspect = Xresolution / std::max(Xresolution, Yresolution);
  float yAspect = Yresolution / std::max(Xresolution, Yresolution);
  transformationData->setAspectRatio(1 / xAspect, 1 / yAspect);

  // Check if the aspect ratios align
  for (SliLayer::Ptr layer : source->layers) {
    if (std::abs((layer->xAspect / layer->yAspect) - (xAspect / yAspect)) >
        1e-3) {
      boost::format warningFormat =
          boost::format("Warning: Aspect ratio mismatch - SLI file defines "
                        "xAspect=%.3f and yAspect=%.3f "
                        "but layer %s has xAspect=%.3f and yAspect=%.3f") %
          xAspect % yAspect % layer->name.c_str() % layer->xAspect %
          layer->yAspect;

      printf("%s\n", warningFormat.str().c_str());
      ShowWarning(warningFormat.str());
    }
  }
  return true;
}

bool SliPresentation::parseSli(const std::string &sliFileName) {
  std::ifstream file(sliFileName);
  std::string line;
  std::regex e("\\s+"); // split on whitespaces
  std::sregex_token_iterator j;
  namespace fs = boost::filesystem;
  std::string dirPath = fs::path(sliFileName).parent_path().string();

  // Iterate over the lines
  while (std::getline(file, line)) {
    std::sregex_token_iterator i(line.begin(), line.end(), e, -1);
    // Iterate over the whitespace-separated tokens of the line
    std::string firstToken = *i++;
    if (firstToken == "Xresolution:" && i != j) {
      Xresolution = std::stof(*i++);
      printf("xresolution: %f\n", Xresolution);
    } else if (firstToken == "Yresolution:" && i != j) {
      Yresolution = std::stof(*i++);
      printf("yresolution: %f\n", Yresolution);
    } else if (firstToken == "varnish_file:") {
      std::string varnishFile = std::string(*i++);
      printf("varnish_file: %s\n", varnishFile.c_str());
      fs::path imagePath = fs::path(dirPath) /= varnishFile;
      if (fs::exists(imagePath)) {
        printf("varnish file exists.\n");
        SliLayer::Ptr varnishLayer =
            SliLayer::create(imagePath.string(), varnishFile, 0, 0);
        if (varnishLayer->fillMetaFromTiff(8, 1)) {
          varnishLayer->fillBitmapFromTiff();
          varnish = Varnish::create(varnishLayer);
        } else {
          std::string error =
              "Error: Varnish file could not be loaded successfully";
          printf("%s\n", error.c_str());
          Show(error, GTK_MESSAGE_ERROR);
          return false;
        }
      } else {
        boost::format errorFormat =
            boost::format("Error: Varnish file not found: %s") %
            imagePath.c_str();
        printf("%s\n", errorFormat.str().c_str());
        Show(errorFormat.str(), GTK_MESSAGE_ERROR);
        return false;
      }
    } else if (fs::exists(fs::path(dirPath) /= firstToken)) {
      // Line contains name of an existing file
      if (i != j && *i == ":") {
        i++; // discard the colon
      }
      int xOffset = 0;
      int yOffset = 0;
      if (i != j) {
        xOffset = std::stoi(*i++);
      }
      if (i != j) {
        yOffset = std::stoi(*i++);
      }
      fs::path imagePath = fs::path(dirPath) /= firstToken;
      if (!source->addLayer(imagePath.string(), firstToken, xOffset, yOffset)) {
        return false;
      }
    } else {
      boost::format errorFormat =
          boost::format(
              "Error: Token '%s' in SLI file is not an existing file") %
          firstToken.c_str();
      printf("%s\n", errorFormat.str().c_str());
      Show(errorFormat.str(), GTK_MESSAGE_ERROR);
      return false;
    }
  }
  if (Xresolution > 0 && Yresolution > 0 && source->layers.size() > 0) {
    source->queryImportBitmaps();
    return true;
  }
  std::string error = "Error: SLI file does not define all required parameters";
  printf("%s\n", error.c_str());
  Show(error, GTK_MESSAGE_ERROR);
  return false;
}

////////////////////////////////////////////////////////////////////////
// SliPresentationInterface

void SliPresentation::wipeCacheAndRedraw() { source->wipeCacheAndRedraw(); }

void SliPresentation::triggerRedraw() {
  for (ViewInterface::WeakPtr view : views) {
    ViewInterface::Ptr viewPtr = view.lock();
    gdk_threads_enter();
    viewPtr->invalidate();
    gdk_threads_leave();
  }
}

boost::dynamic_bitset<> SliPresentation::getVisible() {
  return source->visible;
}

void SliPresentation::setToggled(boost::dynamic_bitset<> bitmap) {
  source->toggled = bitmap;
}

////////////////////////////////////////////////////////////////////////
// PresentationInterface

Scroom::Utils::Rectangle<double> SliPresentation::getRect() {
  GdkRectangle rect;
  rect.x = 0;
  rect.y = 0;
  rect.width = source->total_width;
  rect.height = source->total_height;

  return rect;
}

void SliPresentation::redraw(ViewInterface::Ptr const &vi, cairo_t *cr,
                             Scroom::Utils::Rectangle<double> presentationArea,
                             int zoom) {
  UNUSED(vi);
  GdkRectangle presentArea = presentationArea.toGdkRectangle();
  Scroom::Utils::Rectangle<double> actualPresentationArea = getRect();
  double pixelSize = pixelSizeFromZoom(zoom);

  drawOutOfBoundsWithBackground(cr, presentArea, actualPresentationArea,
                                pixelSize);

  SurfaceWrapper::Ptr surfaceWrap = source->getSurface(zoom);

  // Check if it's not computed yet and we need to draw the waiting rectangle
  if (surfaceWrap == nullptr) {
    drawRectangle(cr, Color(0.5, 1, 0.5),
                  pixelSize *
                      (actualPresentationArea - presentationArea.getTopLeft()));
    return;
  }

  // The level that we need is in the cache, so draw it!
  cairo_save(cr);
  cairo_translate(cr, -presentArea.x * pixelSize, -presentArea.y * pixelSize);
  if (zoom >= 0) {
    // We're using the bottom bitmap, hence we have to scale
    cairo_scale(cr, 1 << zoom, 1 << zoom);
    cairo_set_source_surface(cr, surfaceWrap->surface, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
  } else {
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

bool SliPresentation::getProperty(const std::string &name, std::string &value) {
  std::map<std::string, std::string>::iterator p = properties.find(name);
  bool found = false;
  if (p == properties.end()) {
    found = false;
    value = "";
  } else {
    found = true;
    value = p->second;
  }

  return found;
}

bool SliPresentation::isPropertyDefined(const std::string &name) {
  return properties.end() != properties.find(name);
}

std::string SliPresentation::getTitle() { return filepath; }

////////////////////////////////////////////////////////////////////////
// PresentationBase

void SliPresentation::viewAdded(ViewInterface::WeakPtr vi) {
  controlPanel = SliControlPanel::create(vi, weakPtrToThis);
  controlPanel->disableInteractions();

  // Provide the source with the means to enable and disable the widgets in the
  // sidebar
  source->enableInteractions =
      boost::bind(&SliControlPanel::enableInteractions, controlPanel);
  source->disableInteractions =
      boost::bind(&SliControlPanel::disableInteractions, controlPanel);

  views.insert(vi);

  if (varnish) {
    varnish->setView(vi);
  }
}

void SliPresentation::viewRemoved(ViewInterface::WeakPtr vi) {
  views.erase(vi);
}

std::set<ViewInterface::WeakPtr> SliPresentation::getViews() { return views; }

// ////////////////////////////////////////////////////////////////////////
// // PipetteViewInterface
// ////////////////////////////////////////////////////////////////////////

PipetteLayerOperations::PipetteColor
SliPresentation::getPixelAverages(Scroom::Utils::Rectangle<int> area) {
  if (getArea(area) <= 0)
    return {};

  auto surfaceWrapper = source->getSurface(0);
  int stride = surfaceWrapper->getStride();
  uint8_t *surfaceBegin =
      cairo_image_surface_get_data(source->getSurface(0)->surface);
  Scroom::Utils::Rectangle<int> intersectionPixels =
      area.intersection(surfaceWrapper->toRectangle());
  Scroom::Utils::Rectangle<int> intersectionBytes =
      toBytesRectangle(intersectionPixels);
  int offset = pointToOffset(intersectionBytes.getTopLeft(), stride);
  int offsetEnd =
      pointToOffset(intersectionBytes.getBottomRight(), stride) - stride;
  
  uint8_t A;
  double R, G, B, c, m, y, k;
  double C = 0, Y = 0, M = 0, K = 0;

  for (; offset < offsetEnd; offset += 4) // SPP = 4
  {
    if (offset % stride == intersectionBytes.getRight() % stride)
      offset += stride - intersectionBytes.getWidth();

    B = surfaceBegin[offset + 0];
    G = surfaceBegin[offset + 1];
    R = surfaceBegin[offset + 2];
    A = surfaceBegin[offset + 3];

    c = (255.0 - R);
    m = (255.0 - G);
    y = (255.0 - B);
    k = std::min({c, m, y});

    // transparent -> only the white background of Scroom remains visible
    if (A == 0) {
      C += 0.0;
      M += 0.0;
      Y += 0.0;
      K += 0.0;
    } else {
      C += c - k;
      M += m - k;
      Y += y - k;
      K += k;
    }
  }

  PipetteLayerOperations::PipetteColor result = {
      {"C", C / getArea(intersectionPixels)},
      {"M", M / getArea(intersectionPixels)},
      {"Y", Y / getArea(intersectionPixels)},
      {"K", K / getArea(intersectionPixels)}};

  return result;
}