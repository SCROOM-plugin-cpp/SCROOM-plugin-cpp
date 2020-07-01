#include "seppresentation.hh"

#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>
#include <scroom/tiledbitmaplayer.hh>
#include <string>

#include "sep-helpers.hh"

/////////////////////////////////////////////////////////
///// SepPresentation ///////////////////////////////////

SepPresentation::SepPresentation() : sep_source(SepSource::create()) {
  properties[PIPETTE_PROPERTY_NAME] = ""; // add support for pipette
}

SepPresentation::~SepPresentation() {}

SepPresentation::Ptr SepPresentation::create() {
  return Ptr(new SepPresentation());
}

bool SepPresentation::load(const std::string &file_name) {
  const SepFile file_content = SepSource::parseSep(file_name);
  this->file_name = file_name;

  width = file_content.width;
  height = file_content.height;

  // not knowing width or height is a critical error
  // so we cannot load the file without it
  if (width == 0 || height == 0)
    return false;

  sep_source->setData(file_content);
  sep_source->openFiles();
  sep_source->checkFiles();

  transform = sep_source->getTransform();

  layer_operations = OperationsCMYK32::create();

  tbi = createTiledBitmap(width, height, {layer_operations});
  tbi->setSource(sep_source);

  return true;
}

TransformationData::Ptr SepPresentation::getTransform() { return transform; }

void SepPresentation::triggerRedraw() {
  for (ViewInterface::WeakPtr view : views) {
    ViewInterface::Ptr viewPtr = view.lock();
    gdk_threads_enter();
    viewPtr->invalidate();
    gdk_threads_leave();
  }
}

////////////////////////////////////////////////////////////////////////
// PresentationInterface

Scroom::Utils::Rectangle<double> SepPresentation::getRect() {
  return {0, 0, static_cast<double>(width), static_cast<double>(height)};
}

void SepPresentation::redraw(ViewInterface::Ptr const &vi, cairo_t *cr,
                             Scroom::Utils::Rectangle<double> presentationArea,
                             int zoom) {
  drawOutOfBoundsWithoutBackground(cr, presentationArea, getRect(),
                                   pixelSizeFromZoom(zoom));
  if (tbi)
    tbi->redraw(vi, cr, presentationArea, zoom);

  // Draw varnish if it exists
  if (sep_source->varnish != nullptr) {
    sep_source->varnish->drawOverlay(vi, cr, presentationArea, zoom);
  }
}

bool SepPresentation::getProperty(const std::string &name, std::string &value) {
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

bool SepPresentation::isPropertyDefined(const std::string &name) {
  return properties.end() != properties.find(name);
}

std::string SepPresentation::getTitle() { return file_name; }

////////////////////////////////////////////////////////////////////////
// PresentationBase

void SepPresentation::viewAdded(ViewInterface::WeakPtr interface) {
  if (tbi == nullptr) {
    printf(
        "ERROR: SepPresentation::open(): No TiledBitmapInterface available!\n");
    return;
  }

  if (views.empty() && sep_source->varnish != nullptr) {
    sep_source->varnish->setView(interface);
    sep_source->varnish->triggerRedraw = boost::bind(
        &SepPresentation::triggerRedraw, shared_from_this<SepPresentation>());
  }

  views.insert(interface);

  tbi->open(interface);
}

void SepPresentation::viewRemoved(ViewInterface::WeakPtr interface) {
  views.erase(interface);

  if (tbi == nullptr) {
    printf("ERROR: SepPresentation::close(): No TiledBitmapInterface "
           "available!\n");
    return;
  }

  if (!views.empty() && sep_source->varnish &&
      interface.lock() == sep_source->varnish->viewWeak.lock()) {
    sep_source->varnish->resetView(*views.begin());
  }

  tbi->close(interface);
}

std::set<ViewInterface::WeakPtr> SepPresentation::getViews() { return views; }

////////////////////////////////////////////////////////////////////////
// PipetteViewInterface

PipetteLayerOperations::PipetteColor
SepPresentation::getPixelAverages(Scroom::Utils::Rectangle<int> area) {
  Scroom::Utils::Rectangle<int> presentationArea = getRect().toIntRectangle();
  area = area.intersection(presentationArea);

  Layer::Ptr bottomLayer = tbi->getBottomLayer();
  PipetteLayerOperations::PipetteColor pipetteColors;

  int totalPixels = area.getWidth() * area.getHeight();
  if (totalPixels == 0) {
    return {};
  }

  // Get start tile
  int tile_pos_x_start = area.getLeft() / TILESIZE;
  int tile_pos_y_start = area.getTop() / TILESIZE;

  // Get last tile
  int tile_pos_x_end = (area.getRight() - 1) / TILESIZE;
  int tile_pos_y_end = (area.getBottom() - 1) / TILESIZE;

  for (int x = tile_pos_x_start; x <= tile_pos_x_end; x++) {
    for (int y = tile_pos_y_start; y <= tile_pos_y_end; y++) {
      ConstTile::Ptr tile = bottomLayer->getTile(x, y)->getConstTileSync();
      Scroom::Utils::Rectangle<int> tile_rectangle(0, 0, tile->width,
                                                   tile->height);
      Scroom::Utils::Point<int> base(x * TILESIZE, y * TILESIZE);

      Scroom::Utils::Rectangle<int> inter_rect =
          tile_rectangle.intersection(area - base);

      pipetteColors = sumPipetteColors(
          pipetteColors, layer_operations->sumPixelValues(inter_rect, tile));
    }
  }

  return dividePipetteColors(pipetteColors, totalPixels);
}