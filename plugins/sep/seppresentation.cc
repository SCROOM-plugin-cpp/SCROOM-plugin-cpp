#include "seppresentation.hh"

#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>

SepPresentation::SepPresentation(ScroomInterface::Ptr interface) : scroomInterface(interface)
{
	sepSource = SepSource::create();
}

SepPresentation::SepPresentation()
{
}

SepPresentation::~SepPresentation()
{
}

SepPresentation::Ptr SepPresentation::create(ScroomInterface::Ptr interface)
{
	return Ptr(new SepPresentation(interface));
}

SepPresentation::Ptr SepPresentation::create()
{
	return Ptr(new SepPresentation());
}

/**
 * As SEP file only gives the name of the TIFF files, 
 * we need to find the path in which these TIFF files are located,
 * which is going to be the same as SEP and can be extracted from SEP fileName.
 */
std::string SepPresentation::findPath(std::string sep_directory)
{
	std::string directory;
	const size_t last_slash_idx = sep_directory.rfind('/');
	if (std::string::npos != last_slash_idx)
	{
		// normally this would fail edge cases,
		// but should be fine in this case
		directory = sep_directory.substr(0, last_slash_idx + 1);
	}

	boost::algorithm::trim(directory);

	return directory;
}

/**
 * Parses the content of the file into a vector. 
 */
std::vector<std::string> SepPresentation::parseSep(const std::string &fileName)
{
	std::ifstream file(fileName); // open file stream
	std::string str;

	std::string delimiter = ":";

	// vector contains information from the sep file
	// i.e. dimensions and names of files without tags
	std::vector<std::string> file_content;

	// parse the file into file_content
	while (std::getline(file, str))
	{
		std::string information = str.substr(str.rfind(delimiter) + 1, std::string::npos);
		boost::algorithm::trim(information);
		file_content.push_back(information);
	}

	return file_content;
}

/**
 * This entire function should be rewritten to not use a temporary file and not
 * rely on outdated CMYK support.
 */
bool SepPresentation::load(const std::string &fileName) {
	std::vector<std::string> file_content = this->parseSep(fileName);

	// if file has 6 lines (width, height, CMYK) , then it a simple cmyk
	// otherwise it uses specced channels i.e. CMYKW+
	int channels = file_content.size() - 2;
	int width = std::stoi(file_content[0]);
	int height = std::stoi(file_content[1]);

	// initialize the bitmap array
	byte* image_data = new byte[height * width * channels];

	// iterate through all channels, open the respective TIFF file for each channel and load the data from it
	int channel = 0;
	std::string current_path = SepPresentation::findPath(fileName);
	for (std::size_t i = 2; i < file_content.size(); i++) {
		// fetch the path to the TIFF image, trim it, and open the image
		std::string path = current_path + file_content[i];
		boost::algorithm::trim(path);

		TIFF *tiff = TIFFOpen(path.c_str(), "r");

		if (tiff) {
			const size_t scanLineSize = static_cast<size_t>(TIFFScanlineSize(tiff));
			std::vector<byte> row(scanLineSize);

			for (int h = 0; h < height; h++) {
				TIFFReadScanline(tiff, row.data(), h);

				for (int j = 0; j < width; j++) {
					int index = channel + channels * (j + h * width);
					image_data[index] = row[j];
				}
			}

			// close the connection
			TIFFClose(tiff);
		}

		// move on to the next channel
		channel++;
	}

	//Create a temporary file in which the CMYK TIFF is to be stored
	std::string temp_file = "temp.tif"; // fileName.substr(fileName.rfind('/') + 1, std::string::npos);

	//Establish a writing connection to the temporary file, and pass on suitable parameters (8bps, 4spp, w+h, extras)
	TIFF *image = TIFFOpen(temp_file.c_str(), "w");
	TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(image, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 4);
	TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 1);
	TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);

	//Read the cmyk image data line by line and write the contents onto the temporary file
	byte *cmyk_data = (byte *)malloc(4 * width * (sizeof(byte)));
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			//channels = 4 for cmyk
			for (int channel = 0; channel < 4; channel++)
			{
				cmyk_data[channel + 4 * j] = (image_data[channel + 4 * (j + i * width)]);
			}
		}
		TIFFWriteScanline(image, cmyk_data, i, 0);
	}

	//Close the connection to the temporary file, free up allocated memory
	TIFFClose(image);
	delete[] cmyk_data;
	delete[] image_data;

	//Foad up the cmyk file and delete it afterwards
	// auto result = scroomInterface->loadPresentation(temp_file);
	// remove(temp_file.c_str());

	try
	{
		this->fileName = temp_file;
		tif = TIFFOpen(this->fileName.c_str(), "r");
		if (!tif)
		{
			// Todo: report error
			printf("PANIC: Failed to open file %s\n", fileName.c_str());
			return false;
		}

		uint16 spp_ = 0;
		if (1 != TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp_))
			spp_ = 1; // Default value, according to tiff spec

		// spp == 4 -> CMYK
		// spp == 3 -> RGB
		// spp == 1 -> grayscale
		if (spp_ != 1 && spp_ != 3 && spp_ != 4)
		{
			printf("PANIC: Samples per pixel is neither 1 nor 3 nor 4, but %d. Giving up\n", spp_);
			return false;
		}
		this->spp = spp_;

		// get width and height of the image
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

		// check bps
		uint16 bps_ = 0;
		if (1 != TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps_))
		{
			if (spp == 1)
				bps_ = 1;
			else
				bps_ = 8;
		}
		else
		{
			if (spp == 3)
			{
				if (bps_ != 8)
				{
					printf("PANIC: Bits per sample is not 8, but %d. Giving up\n", bps_);
					return false;
				}
			}
		}
		this->bps = bps_;

		Colormap::Ptr originalColormap;

		uint16 *r, *g, *b;
		int result = TIFFGetField(tif, TIFFTAG_COLORMAP, &r, &g, &b);
		if (result == 1)
		{
			originalColormap = Colormap::create();
			originalColormap->name = "Original";
			size_t count = 1UL << bps;
			originalColormap->colors.resize(count);

			for (size_t i = 0; i < count; i++)
			{
				originalColormap->colors[i] = Color(1.0 * r[i] / 0xFFFF,
													1.0 * g[i] / 0xFFFF, 1.0 * b[i] / 0xFFFF);
			}

			colormapHelper = ColormapHelper::create(originalColormap);
		}

		uint16 photometric;
		TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
		switch (photometric)
		{
		case PHOTOMETRIC_MINISBLACK:
			if (originalColormap)
				printf("WEIRD: Tiff contains a colormap, but photometric isn't palette\n");

			if (bps == 1 || bps == 8)
				colormapHelper = MonochromeColormapHelper::create(2);
			else
				colormapHelper = MonochromeColormapHelper::create(1 << bps);

			properties[MONOCHROME_COLORMAPPABLE_PROPERTY_NAME] = "";
			break;

		case PHOTOMETRIC_MINISWHITE:
			if (originalColormap)
				printf("WEIRD: Tiff contains a colormap, but photometric isn't palette\n");

			if (bps == 1 || bps == 8)
				colormapHelper = MonochromeColormapHelper::createInverted(2);
			else
				colormapHelper = MonochromeColormapHelper::createInverted(1 << bps);

			properties[MONOCHROME_COLORMAPPABLE_PROPERTY_NAME] = "";
			break;

		case PHOTOMETRIC_PALETTE:
			if (!originalColormap)
			{
				printf("WEIRD: Photometric is palette, but tiff doesn't contain a colormap\n");
				colormapHelper = ColormapHelper::create(1 << bps);
			}
			break;

		case PHOTOMETRIC_RGB:
			if (originalColormap)
				printf("WEIRD: Tiff contains a colormap, but photometric isn't palette\n");
			break;

		case PHOTOMETRIC_SEPARATED:
			if (originalColormap)
				printf("WEIRD: Tiff contains a colormap, but photometric isn't palette\n");
			break;

		default:
			printf("PANIC: Unrecognized value for photometric\n");
			return false;
		}

		float resolutionX;
		float resolutionY;
		uint16 resolutionUnit;

		if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &resolutionX) &&
			TIFFGetField(tif, TIFFTAG_YRESOLUTION, &resolutionY) &&
			TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resolutionUnit))
		{
			if (resolutionUnit != RESUNIT_NONE)
			{
				// Fix aspect ratio only
				float base = std::max(resolutionX, resolutionY);
				resolutionX = resolutionX / base;
				resolutionY = resolutionY / base;
			}

			transformationData = TransformationData::create();
			transformationData->setAspectRatio(1 / resolutionX, 1 / resolutionY);
		}
		else
		{
			resolutionX = 1;
			resolutionY = 1;
		}
		printf("This bitmap has size %d*%d, aspect ratio %.1f*%.1f\n",
			   width, height, 1 / resolutionX, 1 / resolutionY);

		LayerSpec ls;
		if (spp == 4 && (bps == 8 || bps == 4 || bps == 2 || bps == 1))
		{
			ls.push_back(OperationsCMYK::create(bps));
		}
		else if (spp == 3 && bps == 8)
		{
			ls.push_back(Operations24bpp::create());
		}
		else if (bps == 2 || bps == 4 || photometric == PHOTOMETRIC_PALETTE)
		{
			ls.push_back(
				Operations::create(colormapHelper, bps));
			ls.push_back(
				OperationsColormapped::create(colormapHelper,
											  bps));
			properties[COLORMAPPABLE_PROPERTY_NAME] = "";
		}
		else if (bps == 1)
		{
			ls.push_back(
				Operations1bpp::create(colormapHelper));
			ls.push_back(
				Operations8bpp::create(colormapHelper));
		}
		else if (bps == 8)
		{
			ls.push_back(
				Operations8bpp::create(colormapHelper));
		}
		else
		{
			printf("PANIC: %d bits per pixel not supported\n", bps);
			return false;
		}

		tbi = createTiledBitmap(width, height, ls);
		tbi->setSource(shared_from_this<SepSource>());

		return true;
	}
	catch (const std::exception &ex)
	{
		printf("PANIC: %s\n", ex.what());
		return false;
	}
}

/** 
 * This function is only needed when the SepPresentation is used by the SliPresentation 
 * to parse and retrieve a layer of an SLI file.
 * Upon being called, it fills the bitmap and all other relevant attributes of the SliLayer.
*/
void SepPresentation::fillSliLayer(SliLayer::Ptr sliLayer)
{

}

////////////////////////////////////////////////////////////////////////
// PresentationInterface

Scroom::Utils::Rectangle<double> SepPresentation::getRect()
{
	GdkRectangle rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;

	return rect;
}

void SepPresentation::redraw(ViewInterface::Ptr const &vi, cairo_t *cr,
							 Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
	drawOutOfBoundsWithoutBackground(cr, presentationArea, getRect(), pixelSizeFromZoom(zoom));
	if (tbi)
		tbi->redraw(vi, cr, presentationArea, zoom);

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
	return "temp.tif";
}

////////////////////////////////////////////////////////////////////////
// PresentationBase

void SepPresentation::viewAdded(ViewInterface::WeakPtr viewInterface)
{
	views.insert(viewInterface);

	if (tbi)
		tbi->open(viewInterface);
	else
	{
		printf("ERROR: SepPresentation::open(): No TiledBitmapInterface available!\n");
	}
}

void SepPresentation::viewRemoved(ViewInterface::WeakPtr vi)
{
	views.erase(vi);

	if (tbi)
		tbi->close(vi);
	else
	{
		printf("ERROR: SepPresentation::close(): No TiledBitmapInterface available!\n");
	}
}

std::set<ViewInterface::WeakPtr> SepPresentation::getViews()
{
	return views;
}

////////////////////////////////////////////////////////////////////////////////
// SepSource
////////////////////////////////////////////////////////////////////////////////

SepSource::SepSource()
{
}

SepSource::~SepSource()
{
}

SepSource::Ptr SepSource::create()
{
	return Ptr(new SepSource());
}

void SepSource::fillTiles(int startLine, int lineCount, int tileWidth, int firstTile, std::vector<Tile::Ptr> &tiles)
{
	TIFF *tif = TIFFOpen("temp.tif", "r");
	const uint32_t startLine_ = static_cast<uint32_t>(startLine);
	const size_t firstTile_ = static_cast<size_t>(firstTile);
	const size_t scanLineSize = static_cast<size_t>(TIFFScanlineSize(tif));
	int bps;
	int spp;
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
	const size_t tileStride = static_cast<size_t>(tileWidth * spp * bps / 8);
	std::vector<byte> row(scanLineSize);

	const size_t tileCount = tiles.size();
	auto dataPtr = std::vector<byte *>(tileCount);
	for (size_t tile = 0; tile < tileCount; tile++) {
		dataPtr[tile] = tiles[tile]->data.get();
	}

	for (size_t i = 0; i < static_cast<size_t>(lineCount); i++) {
		TIFFReadScanline(tif, row.data(), static_cast<uint32>(i) + startLine_);

		for (size_t tile = 0; tile < tileCount - 1; tile++)
		{
			memcpy(dataPtr[tile],
				   row.data() + (firstTile_ + tile) * tileStride,
				   tileStride);
			dataPtr[tile] += tileStride;
		}
		memcpy(dataPtr[tileCount - 1],
			   row.data() + (firstTile_ + tileCount - 1) * tileStride,
			   scanLineSize - (firstTile_ + tileCount - 1) * tileStride);
		dataPtr[tileCount - 1] += tileStride;
	}
}

void SepSource::done() {
	remove("temp.tif");
}