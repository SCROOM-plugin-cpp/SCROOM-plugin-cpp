#include "seppresentation.hh"

SepPresentation::SepPresentation(ScroomInterface::Ptr scroomInterface_) : scroomInterface(scroomInterface_)
{
	sepSource = SepSource::create();
}

SepPresentation::SepPresentation()
{
}

SepPresentation::~SepPresentation()
{
}

SepPresentation::Ptr SepPresentation::create(ScroomInterface::Ptr scroomInterface_)
{
	return Ptr(new SepPresentation(scroomInterface_));
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
		// normally this would fail edge cases, but since we only get passed a
		// path with sep file at the end, this should be fine
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
	std::string str;			  // just a temporary variable

	// maps <property> -> <value>
	// where <property> is width, height or channels
	std::vector<std::string> file_content;

	// parse the file into file_content
	int line = 0;
	while (std::getline(file, str))
	{
		// std::cout << file_content[line] << "\n";
		if (line >= 2)
		{
			if (str.compare(4, 1, " ") != 0)
				file_content.push_back(str.substr(4, std::string::npos));
			else
				file_content.push_back(str.substr(5, std::string::npos));
		}
		else
		{
			file_content.push_back(str);
		}
		line++;
	}

	return file_content;
}

bool SepPresentation::load(const std::string &fileName)
{
	int index = fileName.rfind('.');
	if (fileName.substr(index + 1, std::string::npos) == "sep")
		printf("sep file is detected\n");
	else
		printf("sli file is detected\n");

	std::vector<std::string> file_content = SepPresentation::parseSep(fileName);

	// if file has 6 lines (width, height, CMYK) , then it a simple cmyk
	// otherwise it uses specced channels i.e. CMYKW+
	int channels = file_content.size() - 2;
	int height = (int)std::stoi(file_content[1]);
	int width = (int)std::stoi(file_content[0]);

	// initialize the bitmap array
	int *image_data = new int[height * width * channels];
	for (int i = 0; i < height * width; i++)
	{
		image_data[i] = NULL;
	}

	int channel = 0;
	for (int i = 2; i < file_content.size(); i++)
	{
		std::string current_path = SepPresentation::findPath(fileName);
		std::string path = (std::string)current_path + file_content[i];
		boost::algorithm::trim(path);
		// tifs.push_back(path);
		TIFF *tif = TIFFOpen(path.c_str(), "r");
		if (tif)
		{
			const size_t scanLineSize = static_cast<size_t>(TIFFScanlineSize(tif));
			std::vector<byte> row(scanLineSize);

			uint32 imagelength;

			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imagelength);
			for (int i = 0; i < imagelength; i++)
			{
				TIFFReadScanline(tif, row.data(), i);
				for (int j = 0; j < width; j++)
				{
					int index = channel + channels * (j + i * width);
					image_data[index] = (int)row[j];
				}
			}
		}
		TIFFClose(tif);
		channel++;
	}
	// auto result = host->loadPresentation((std::string)path);

	delete[] image_data;

	tbi = createTiledBitmap(width, height, );
	tbi->setSource(shared_from_this<SourcePresentation>());

	return false;
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
	return fileName;
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
	TIFF *tif = TIFFOpen("/home/ubuntu/Documents/CMYK_Triangles.tif", "r");
	const uint32 startLine_ = static_cast<uint32>(startLine);
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
	for (size_t tile = 0; tile < tileCount; tile++)
	{
		dataPtr[tile] = tiles[tile]->data.get();
	}

	for (size_t i = 0; i < static_cast<size_t>(lineCount); i++)
	{
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

void SepSource::done()
{
	// TODO
}