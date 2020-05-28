#include "sep.hh"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <fstream>
#include <string>
#include <tiffio.h>
#include <iostream>
#include <map>
#include <boost/algorithm/string/trim.hpp>
#include "scroom/transformpresentation.hh"
#include "scroom/tile.hh"

// #include "tiff.hh"
// #include "../../scroom/gui/src/view.hh"
#include "../../../scroom/gui/src/loader.hh"

#include "seppresentation.hh"

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

Sep::Sep()
{
}

Sep::~Sep()
{
}

Sep::Ptr Sep::create()
{
	return Ptr(new Sep());
}

std::string Sep::getPluginName()
{
	return "SEP";
}

std::string Sep::getPluginVersion()
{
	return "0.0";
}

////////////////////////////////////////////////////////////////////////
// PluginInformationInterface
////////////////////////////////////////////////////////////////////////

void Sep::registerCapabilities(ScroomPluginInterface::Ptr host)
{
	host->registerOpenPresentationInterface("SEP viewer", shared_from_this<Sep>());
	this->host = host;
}

////////////////////////////////////////////////////////////////////////
// OpenPresentationInterface
////////////////////////////////////////////////////////////////////////

std::list<GtkFileFilter *> Sep::getFilters()
{
	std::list<GtkFileFilter *> result;

	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "SEP files");
	// gtk_file_filter_add_mime_type(filter, "image/tiff"); // i think this has to be text/plain but not sure if that would cause any problems
	gtk_file_filter_add_pattern(filter, "*.sep"); // maybe this one is better?
	result.push_back(filter);

	return result;
}

/**
 * As SEP file only gives the name of the TIFF files, 
 * we need to find the path in which these TIFF files are located,
 * which is going to be the same as SEP and can be extracted from SEP fileName.
 */
std::string Sep::findPathToTiff(std::string sep_directory)
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
std::map<std::string, std::string> Sep::parseSep(const std::string &fileName)
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

PresentationInterface::Ptr Sep::open(const std::string &fileName)
{
	std::map<std::string, std::string> file_content = Sep::parseSep(fileName);

	// if file has 6 lines (width, height, CMYK) , then it a simple cmyk
	// otherwise it uses specced channels i.e. CMYKW+
	bool simple_channels = file_content.size() == 6;

	std::string current_path = Sep::findPathToTiff(fileName);

	std::string path = (std::string)current_path + file_content["M"];

	boost::algorithm::trim(path);

	std::cout << "what's up" << std::endl;
	std::cout << path << std::endl;

	tdata_t buf;
	TIFF *tif = TIFFOpen(path.c_str(), "r");
	if (tif)
	{
		uint32 imagelength;
		uint32 row;

		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imagelength);
		buf = _TIFFmalloc(TIFFScanlineSize(tif));
		for (row = 0; row < imagelength; row++)
			TIFFReadScanline(tif, buf, row);
		_TIFFfree(buf);
		TIFFClose(tif);
	}

	std::cout << buf << std::endl;

	std::cout << tif << "\n";

	auto result = host->loadPresentation((std::string)path);

	return result;
}
