#include "seppresentation.hh"

#include <scroom/cairo-helpers.hh>
#include <scroom/layeroperations.hh>
#include <gtk/gtk.h>
#include <tiffio.h>
#include <fstream>
#include <string>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/split.hpp>

////////////////////////////////////////////////////////////
////// Container class for messages to be displayed via displayErrorDialog()

ErrorMessage::ErrorMessage(std::string msg, std::string color /*= "red"*/)
{
	this->insert(msg);
}

ErrorMessage::~ErrorMessage()
{
}

bool ErrorMessage::insert(std::string msg)
{
	this->message += msg;
	// avoid inserting unnessary newlines
	if (!msg.empty() && msg.back() != '\n')
		this->message += "\n";

	return true;
}

std::string ErrorMessage::getMessage()
{
	return this->message;
}

/**
 * Display an error dialog with given message and message type.
 */
void displayErrorDialog(std::string message, GtkMessageType type_gtk = GTK_MESSAGE_WARNING)
{
	auto dialog = gtk_message_dialog_new(nullptr,
										 GTK_DIALOG_DESTROY_WITH_PARENT,
										 type_gtk,
										 GTK_BUTTONS_CLOSE,
										 message.c_str());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/////////////////////////////////////////////////////////
///// SepPresentation

SepPresentation::SepPresentation(ScroomInterface::Ptr interface) : scroomInterface(interface)
{
	sep_source = SepSource::create();
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
	// theoretically hardcoding forward slash should be fine since
	// Windows does accept it, but needs to be tested
	return boost::filesystem::path{sep_directory}.parent_path().string() + "/";
}

/**
 * Parses the content of the SEP file.
 * Also displays error dialog when an error in the input file is detected.
 */
SepFile SepPresentation::parseSep(const std::string &fileName)
{
	std::ifstream file(fileName); // open file stream
	std::string str;

	bool warning_error = false; // to see if warning dialog should be displayed

	const std::string delimiter = ":";

	const std::string parent_dir = SepPresentation::findPath(fileName);

	ErrorMessage error(""); // might be overkill now, but originally had a purpose

	SepFile sepfile;

	try
	{
		std::getline(file, str);
		sepfile.width = std::stoul(str);

		std::getline(file, str);
		sepfile.height = std::stoul(str);
	}
	catch (const std::exception &e)
	{
		// print error in red
		error.insert("PANIC: Width or height have not been provided correctly! \n");
		warning_error = true;
		// missing width or height is a fatal error as we cannot open the file without it
		this->fatal_error = true;
	}

	while (std::getline(file, str))
	{
		std::vector<std::string> result;
		boost::split(result, str, boost::is_any_of(delimiter));
		boost::algorithm::trim(result[0]);
		boost::algorithm::trim(result[1]);

		if (result.size() != 2 || result[1] == "")
		{
			// print error in red
			warning_error = true;
			error.insert("PANIC: One of the channels has not been provided correctly! \n");
			continue; // skip loading this channel
		}

		sepfile.files.insert(std::make_pair(result[0], parent_dir + result[1]));
	}

	file.close(); // close file stream

	if (warning_error || this->fatal_error)
	{
		std::cerr << "\033[1;31m" << error.getMessage() << "\033[0m\n";
		::displayErrorDialog(error.getMessage());
	}
	warning_error = false;

	return sepfile;
}

/**
 * Checks whether the input file contains the required values.
 * 
 * TODO: Consider alternatives to the 'count' function
 */
bool SepPresentation::checkFile(const std::map<std::string, std::string> content)
{
	return content.count("C") && content.count("M") && content.count("Y") && content.count("K");
	// && content.count("width") && content.count("height");
}

/**
 * TODO: check if this function meets the requirements.
 */
bool SepPresentation::load(const std::string &file_name)
{
	const auto sepfile = parseSep(file_name);
	const auto file_content = sepfile.files;
	this->file_name = file_name;

	if (this->fatal_error)
	{
		this->fatal_error = false;
		return false;
	}

	// I commented this out for 2 reasons:
	// 1. most of this error handling is already done in parser, where it makes more to me imo
	// 2. checkFile() actually violates our URD

	// Verify integrity of the file
	// if (!checkFile(file_content))
	// {
	// 	printf("PANIC: Missing C, M, Y, K, width or height in SEP file from '%s'.\n", file_name.c_str());
	// 	return false;
	// }

	// Remember whether we have varnish
	const bool has_varnish = file_content.count("V") == 1;

	// We have CMYK as channels - maybe this also needs extending when
	// LC / LM are added.
	const size_t nr_channels = 4;

	this->width = sepfile.width;
	this->height = sepfile.height;

	// Allocate an area for the full image
	auto combined_data = boost::shared_ptr<byte>(new byte[width * height * nr_channels]);
	boost::shared_ptr<byte> varnish_data;
	if (has_varnish)
	{
		varnish_data = boost::shared_ptr<byte>(new byte[width * height]);
	}

	// iterate through all channels, open the respective TIFF file for each channel and load the data from it
	std::string current_path = findPath(file_name);

	size_t index = 0;
	for (auto channel : {"C", "M", "Y", "K"})
	{
		std::string path = file_content.at(channel);

		// Attempt to open the tiff image
		TIFF *tiff = TIFFOpen(path.c_str(), "r");

		if (tiff == nullptr)
		{
			printf("Warning: Could not open channel '%s' from file '%s'.\n", channel, path.c_str());
			continue;
		}

		const size_t line_width = static_cast<size_t>(TIFFScanlineSize(tiff));
		std::vector<byte> row(line_width);

		for (size_t h = 0; h < height; h++)
		{
			TIFFReadScanline(tiff, row.data(), h);

			int base = index + h * nr_channels * width;
			for (size_t j = 0; j < width; j++, base += nr_channels)
			{
				combined_data.get()[base] = row[j];
			}
		}

		TIFFClose(tiff);
		index++;
	}

	if (has_varnish)
	{
		// TODO: maybe move this code?
		std::string path = current_path + file_content.at("V");
		boost::algorithm::trim(path);

		// Attempt to open the tiff image
		TIFF *tiff = TIFFOpen(path.c_str(), "r");

		if (tiff == nullptr)
		{
			printf("PANIC: Could not open channel 'V' from file '%s'.\n", path.c_str());
			return false;
		}

		const size_t line_width = static_cast<size_t>(TIFFScanlineSize(tiff));
		std::vector<byte> row(line_width);

		for (size_t h = 0; h < height; h++)
		{
			TIFFReadScanline(tiff, row.data(), h);
			memcpy(varnish_data.get() + h * width, row.data(), width);
		}

		TIFFClose(tiff);
	}

	// TODO: Support different resolutions
	float resolutionX = 1.0f;
	float resolutionY = 1.0f;

	printf("The combined bitmap has size %ld*%ld, aspect ratio %.1f*%.1f\n",
		   this->width, this->height, 1 / resolutionX, 1 / resolutionY);

	this->sep_source->setData(combined_data, this->width);

	this->tbi = createTiledBitmap(width, height, {OperationsCMYK32::create()});
	this->tbi->setSource(this->sep_source);

	return true;
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
