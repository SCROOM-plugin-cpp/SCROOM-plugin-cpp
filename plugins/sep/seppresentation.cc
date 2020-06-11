#include "seppresentation.hh"

#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>
#include <string>

/////////////////////////////////////////////////////////
///// SepPresentation

SepPresentation::SepPresentation(ScroomInterface::Ptr interface)
	: scroom_interface(interface), sep_source(SepSource::create())
{
}

SepPresentation::~SepPresentation()
{
}

SepPresentation::Ptr SepPresentation::create(ScroomInterface::Ptr interface)
{
	return Ptr(new SepPresentation(interface));
}

/**
 * TODO: check if this function meets the requirements.
 * TODO: Support different resolutions.
 * TODO: Support varnish.
 * TODO: Support pipette.
 */
bool SepPresentation::load(const std::string &file_name)
{
	const SepFile file_content = SepSource::parseSep(file_name);
	this->file_name = file_name;

	this->width = file_content.width;
	this->height = file_content.height;

	if (this->width == 0 || this->height == 0)
		return false;

	this->sep_source->setData(file_content);
	this->sep_source->openFiles();

	this->transform = this->sep_source->getTransform();

	this->tbi = createTiledBitmap(this->width, this->height, {OperationsCMYK32::create()});
	this->tbi->setSource(this->sep_source);

	return true;
}

TransformationData::Ptr SepPresentation::getTransform() {
	return this->transform;
}

////////////////////////////////////////////////////////////////////////
// PresentationInterface

Scroom::Utils::Rectangle<double> SepPresentation::getRect()
{
	return {0, 0, static_cast<double>(this->width), static_cast<double>(this->height)};
}

void SepPresentation::redraw(ViewInterface::Ptr const &vi, cairo_t *cr,
							 Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
	drawOutOfBoundsWithoutBackground(cr, presentationArea, getRect(), pixelSizeFromZoom(zoom));
	if (this->tbi)
		this->tbi->redraw(vi, cr, presentationArea, zoom);
}

bool SepPresentation::getProperty(const std::string &name, std::string &value)
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

bool SepPresentation::isPropertyDefined(const std::string &name)
{
	return properties.end() != properties.find(name);
}

std::string SepPresentation::getTitle()
{
	return this->file_name;
}

////////////////////////////////////////////////////////////////////////
// PresentationBase

void SepPresentation::viewAdded(ViewInterface::WeakPtr interface)
{
	this->views.insert(interface);

	if (this->tbi == nullptr)
	{
		printf("ERROR: SepPresentation::open(): No TiledBitmapInterface available!\n");
		return;
	}

	this->tbi->open(interface);
}

void SepPresentation::viewRemoved(ViewInterface::WeakPtr interface)
{
	this->views.erase(interface);

	if (this->tbi == nullptr)
	{
		printf("ERROR: SepPresentation::close(): No TiledBitmapInterface available!\n");
		return;
	}

	this->tbi->close(interface);
}

std::set<ViewInterface::WeakPtr> SepPresentation::getViews()
{
	return this->views;
}
