#include "seppresentation.hh"

#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>
#include <scroom/tiledbitmaplayer.hh>
#include <string>

#include "sep-helpers.hh"

/////////////////////////////////////////////////////////
///// SepPresentation ///////////////////////////////////

SepPresentation::SepPresentation(ScroomInterface::Ptr interface)
    : scroom_interface(interface), sep_source(SepSource::create()) {
}

SepPresentation::~SepPresentation() {
}

SepPresentation::Ptr SepPresentation::create(ScroomInterface::Ptr interface) {
    return Ptr(new SepPresentation(interface));
}

/**
 * TODO: Support pipette.
 */
bool SepPresentation::load(const std::string &file_name) {
    const SepFile file_content = SepSource::parseSep(file_name);
    this->file_name = file_name;

    this->width = file_content.width;
    this->height = file_content.height;

    // not knowing width or height is a critical error
    // so we cannot load the file without it
    if (this->width == 0 || this->height == 0)
        return false;

    this->sep_source->setData(file_content);
    this->sep_source->openFiles();
    this->sep_source->checkFiles();

    this->transform = this->sep_source->getTransform();

    this->layer_operations = OperationsCMYK32::create();

    this->tbi = createTiledBitmap(this->width, this->height, {this->layer_operations});
    this->tbi->setSource(this->sep_source);

    return true;
}

TransformationData::Ptr SepPresentation::getTransform() {
    return this->transform;
}

////////////////////////////////////////////////////////////////////////
// PresentationInterface

Scroom::Utils::Rectangle<double> SepPresentation::getRect() {
    return {0, 0, static_cast<double>(this->width), static_cast<double>(this->height)};
}

void SepPresentation::redraw(ViewInterface::Ptr const &vi, cairo_t *cr,
                             Scroom::Utils::Rectangle<double> presentationArea, int zoom) {
    drawOutOfBoundsWithoutBackground(cr, presentationArea, getRect(), pixelSizeFromZoom(zoom));
    if (this->tbi)
        this->tbi->redraw(vi, cr, presentationArea, zoom);

    // Draw varnish if it exists
    if (this->sep_source->varnish != nullptr) {
        this->sep_source->varnish->drawOverlay(vi, cr, presentationArea, zoom);
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

std::string SepPresentation::getTitle() {
    return this->file_name;
}

////////////////////////////////////////////////////////////////////////
// PresentationBase

void SepPresentation::viewAdded(ViewInterface::WeakPtr interface) {
    this->views.insert(interface);

    if (this->tbi == nullptr) {
        printf("ERROR: SepPresentation::open(): No TiledBitmapInterface available!\n");
        return;
    }

    this->tbi->open(interface);

    if (this->sep_source->varnish != nullptr) {
        this->sep_source->varnish->setView(interface);
    }
}

void SepPresentation::viewRemoved(ViewInterface::WeakPtr interface) {
    this->views.erase(interface);

    if (this->tbi == nullptr) {
        printf("ERROR: SepPresentation::close(): No TiledBitmapInterface available!\n");
        return;
    }

    this->tbi->close(interface);
}

std::set<ViewInterface::WeakPtr> SepPresentation::getViews() {
    return this->views;
}

////////////////////////////////////////////////////////////////////////
// PipetteViewInterface

PipetteLayerOperations::PipetteColor SepPresentation::getPixelAverages(Scroom::Utils::Rectangle<int> area) {
    Scroom::Utils::Rectangle<int> presentationArea = this->getRect().toIntRectangle();
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
            Scroom::Utils::Rectangle<int> tile_rectangle(0, 0, tile->width, tile->height);
            Scroom::Utils::Point<int> base(x * TILESIZE, y * TILESIZE);

            Scroom::Utils::Rectangle<int> inter_rect = tile_rectangle.intersection(area - base);

            pipetteColors = sumPipetteColors(pipetteColors, this->layer_operations->sumPixelValues(inter_rect, tile));
        }
    }

    return dividePipetteColors(pipetteColors, totalPixels);
}