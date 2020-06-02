#include "seppresentation.hh"

SepPresentation::SepPresentation(ScroomInterface::Ptr scroomInterface_): scroomInterface(scroomInterface_)
{
  sepSource = SepSource::create();
}

SepPresentation::~SepPresentation()
{
}

SepPresentation::Ptr SepPresentation::create(ScroomInterface::Ptr scroomInterface_)
{
  return Ptr(new SepPresentation(scroomInterface_));
}

/**
 * As SEP file only gives the name of the TIFF files, 
 * we need to find the path in which these TIFF files are located,
 * which is going to be the same as SEP and can be extracted from SEP fileName.
 */
std::string SepPresentation::findPathToTiff(std::string sep_directory)
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
 * Parses the content of the file into a map. 
 */
std::map<std::string, std::string> SepPresentation::parseSep(const std::string &fileName)
{
	std::ifstream file(fileName); // open file stream
	std::string str;			  // just a temporary variable

	// maps <property> -> <value>
	// where <property> is width, height or channels
	std::map<std::string, std::string> file_content;

	// parse the file into file_content
	int line = 0;
	while (std::getline(file, str))
	{
		// std::cout << file_content[line] << "\n";
		if (line == 0)
		{
			file_content.insert(std::make_pair("width", str));
		}
		else if (line == 1)
		{
			file_content.insert(std::make_pair("height", str));
		}
		else if (line >= 2)
		{
			if (str.compare(4, 1, " ") != 0)
				file_content.insert(std::make_pair(str.substr(0, 1), str.substr(4, std::string::npos)));
			else
				file_content.insert(std::make_pair(str.substr(0, 2), str.substr(5, std::string::npos)));
		}

		line++;
	}

	return file_content;
}

bool SepPresentation::load(const std::string& fileName)
{
  std::map<std::string, std::string> file_content = SepPresentation::parseSep(fileName);

  // For getting a presentation from the TIFF plugin:
  //PresentationInterface::Ptr tiffPresentation = scroomInterface->loadPresentation(fileName);

	// if file has 6 lines (width, height, CMYK) , then it a simple cmyk
	// otherwise it uses specced channels i.e. CMYKW+
	int channels = file_content.size() - 2;
	int height = (int)std::stoi(file_content["height"]);
	int width = (int)std::stoi(file_content["width"]);

	//int* image_data[][][] = int[height][width][channels];

	std::map<std::string, std::string>::iterator it = file_content.begin();
	it++;
	it++;
	int channel = 0;

	std::cout << "joe mama" << std::endl;

	while (it != file_content.end())
	{
		std::string current_path = SepPresentation::findPathToTiff(fileName);
		std::string path = (std::string)current_path + it->second;
		boost::algorithm::trim(path);
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
					//image_data[i][j][channel] = row[j];
				}
			}
		}
		TIFFClose(tif);
		channel++;
		it++;
	}

	//std::cout << image_data[3000][500][1] << "\n";
	// auto result = host->loadPresentation((std::string)path);

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

void SepPresentation::redraw(ViewInterface::Ptr const& vi, cairo_t* cr,
    Scroom::Utils::Rectangle<double> presentationArea, int zoom)
{
  // TODO
}

bool SepPresentation::getProperty(const std::string& name, std::string& value)
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

bool SepPresentation::isPropertyDefined(const std::string& name)
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

void SepSource::fillTiles(int startLine, int lineCount, int tileWidth, int firstTile, std::vector<Tile::Ptr>& tiles)
{
  // TODO
}

void SepSource::done()
{
  // TODO
}