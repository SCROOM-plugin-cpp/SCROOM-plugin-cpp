#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

#include "sepsource.hh"

enum MSG_TYPE
{
	INFO,
	WARNING,
	ERROR
};

void showWarning(MSG_TYPE type, std::string message)
{
	GtkMessageType type_gtk;
	if (type == MSG_TYPE::INFO)
	{
		type_gtk = GTK_MESSAGE_INFO;
	}
	else if (type == MSG_TYPE::WARNING)
	{
		type_gtk = GTK_MESSAGE_WARNING;
	}
	else
	{
		type_gtk = GTK_MESSAGE_ERROR;
	}

	// We don't have a pointer to the parent window, so we'll just supply nullptr..
	GtkWidget *dialog = gtk_message_dialog_new(
		nullptr, GTK_DIALOG_DESTROY_WITH_PARENT,
		type_gtk, GTK_BUTTONS_CLOSE, message.c_str());

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

SepSource::SepSource() {}
SepSource::~SepSource() {}

SepSource::Ptr SepSource::create()
{
	return Ptr(new SepSource());
}

/**
 * As SEP file only gives the file name of the TIFF files, we need to find the
 * path in which these TIFF files are located to construct the full path. This
 * function returns the path of the parent folder of the loaded SEP file.
 */
std::string SepSource::findPath(const std::string &sep_directory)
{
	// theoretically hardcoding forward slash should be fine since
	// Windows does accept it, but needs to be tested
	return boost::filesystem::path{sep_directory}.parent_path().string() + "/";
}

/**
 * Parses the content of a given SEP file.
 * When the width or height are not properly specified or one of the channel names
 * is empty or one of the channel lines does not follow the specification, an
 * warning dialog is shown.
 */
SepFile SepSource::parseSep(const std::string &file_name)
{
	std::ifstream file(file_name);
	const std::string delimiter = ":";
	const std::string parent_dir = SepSource::findPath(file_name);

	SepFile sep_file;
	std::string str;
	std::string errors = "";

	try
	{
		std::getline(file, str);
		sep_file.width = std::stoul(str);
		std::getline(file, str);
		sep_file.height = std::stoul(str);
	}
	catch (const std::exception &e)
	{
		errors += "PANIC: Width or height have not been provided correctly!\n";
	}

	// Make sure the required channels exist (albeit with empty paths). We
	// can then later check for this value as an error value.
	sep_file.files = {{"C", ""}, {"M", ""}, {"Y", ""}, {"K", ""}};

	while (std::getline(file, str))
	{
		std::vector<std::string> result;
		boost::split(result, str, boost::is_any_of(delimiter));
		boost::algorithm::trim(result[0]);
		boost::algorithm::trim(result[1]);

		if (result.size() != 2 || result[1].empty() )
		{
			// Remember the warning and skip this line
			errors += "PANIC: One of the channels has not been provided correctly!\n";
			continue;
		}
		std::cout << result[0] + "\t" << parent_dir + result[1] << std::endl;
		sep_file.files[result[0]] = parent_dir + result[1];
	}

	// Close the file
	file.close();

	if (!errors.empty())
	{
		std::cerr << errors;
		showWarning(WARNING, errors);
	}

	return sep_file;
}

void SepSource::fillSliLayer(SliLayer::Ptr sli)
{
	if (sli->filepath.empty())
	{
		return;
	}

	const SepFile values = SepSource::parseSep(sli->filepath);

	sli->height = values.height;
	sli->width = values.width;

	sli->spp = 4;
	sli->bps = 8;

	const int row_width = sli->width * 4; // 4 bytes per pixel
	sli->bitmap = new uint8_t[sli->height * row_width];

	auto source = SepSource::create();
	// source->setData(values);
	source->openFiles(values);

	auto temp = std::vector<byte>(row_width);
	for (int y = 0; y < sli->height; y++)
	{
		source->readCombinedScanline(temp, y);
		memcpy(sli->bitmap + y * row_width, temp.data(), row_width);
	}

	source->done();
}

void SepSource::setData(SepFile sep_file_)
{
	
}

void SepSource::openFiles(SepFile sep_file_)
{
	this->sep_file = sep_file_;
	if (this->file_c != NULL && this->file_m != NULL && this->file_y != NULL && this->file_k != NULL)
	{
		// why is this being printed?
		printf("PANIC: PANIC\n");
		return;
	}

	this->file_c = TIFFOpen(this->sep_file.files["C"].c_str(), "r");
	this->file_m = TIFFOpen(this->sep_file.files["M"].c_str(), "r");
	this->file_y = TIFFOpen(this->sep_file.files["Y"].c_str(), "r");
	this->file_k = TIFFOpen(this->sep_file.files["K"].c_str(), "r");

	if (this->sep_file.files.count("W"))
		this->file_w = TIFFOpen(this->sep_file.files["W"].c_str(), "r");

	if (this->file_c == NULL || this->file_m == NULL || this->file_y == NULL || this->file_k == NULL || this->file_w == NULL)
	{
		std::string error = "PANIC: One of the provided files is not valid, or could not be opened!\n";
		printf(error.c_str());
		showWarning(WARNING, error);
	}
}

int TIFFReadScanline_(tiff *file, void *buf, uint32 row, uint16 sample = 0)
{
	if (file)
	{
		return TIFFReadScanline(file, buf, row, sample);
	}
	return -1;
}

void SepSource::readCombinedScanline(std::vector<byte> &out, size_t line_nr)
{
	// There are 4 channels in out, so the number of bytes an individual
	// channel has is one fourth of the output vector's size.
	size_t size = out.size() / 4;

	// Create buffers for the scanlines of the individual channels.
	auto c_line = std::vector<byte>(size);
	auto m_line = std::vector<byte>(size);
	auto y_line = std::vector<byte>(size);
	auto k_line = std::vector<byte>(size);
	auto w_line = std::vector<byte>(size);

	// Read scanlines of the individual channels.
	TIFFReadScanline_(this->file_c, c_line.data(), line_nr);
	TIFFReadScanline_(this->file_m, m_line.data(), line_nr);
	TIFFReadScanline_(this->file_y, y_line.data(), line_nr);
	TIFFReadScanline_(this->file_k, k_line.data(), line_nr);
	TIFFReadScanline_(this->file_w, w_line.data(), line_nr);

	// Merge the scanlines of individual channels: first C, then M,
	// then Y and finally K.
	for (size_t i = 0; i < size; i++)
	{
		out[4 * i + 0] = c_line[i] - w_line[i];
		out[4 * i + 1] = m_line[i] - w_line[i];
		out[4 * i + 2] = y_line[i] - w_line[i];
		out[4 * i + 3] = k_line[i] - w_line[i];
	}
}

void SepSource::fillTiles(int startLine, int line_count, int tileWidth, int firstTile, std::vector<Tile::Ptr> &tiles)
{
	const size_t bpp = 4; // number of bytes per pixel
	const size_t start_line = static_cast<size_t>(startLine);
	const size_t first_tile = static_cast<size_t>(firstTile);
	const size_t tile_stride = static_cast<size_t>(tileWidth) * bpp;
	const size_t tile_count = tiles.size();

	// Open the TIFF files so we can read from them later.
	// this->openFiles();

	// Buffer for the scanline to be written into
	auto row = std::vector<byte>(bpp * this->sep_file.width);

	//
	auto tile_data = std::vector<byte *>(tile_count);
	for (size_t tile = 0; tile < tile_count; tile++)
	{
		tile_data[tile] = tiles[tile]->data.get();
	}

	// The number of bytes that are in the full tiles of the image
	const size_t accounted_width = (first_tile + tile_count - 1) * tile_stride;

	// The number of remaining bytes
	const size_t remaining_width = bpp * this->sep_file.width - accounted_width;

	// This points to the beginning of the row (taking the starting tile
	// into account).
	const byte *horizontal_offset = row.data() + first_tile * tile_stride;
	std::cout << horizontal_offset << std::endl;

	for (size_t i = 0; i < static_cast<size_t>(line_count); i++)
	{
		this->readCombinedScanline(row, i + start_line);

		// The general case for completely filled tiles. The last tile
		// is the only tile that might not be completely filled, so that
		// case has a separate implementation below.
		for (size_t tile = 0; tile < tile_count - 1; tile++)
		{
			memcpy(tile_data[tile], horizontal_offset + tile * tile_stride, tile_stride);
			tile_data[tile] += tile_stride;
		}

		// Copy the data into the last tile. This tile might not be
		// completely filled, so that's why this case is not included
		// in the for loop.
		memcpy(tile_data[tile_count - 1], row.data() + accounted_width, remaining_width);
		tile_data[tile_count - 1] += tile_stride;
	}
}

void SepSource::done()
{
	// Close all tiff files and reset pointers
	if (this->file_c != nullptr)
	{
		TIFFClose(this->file_c);
		this->file_c = nullptr;
	}
	if (this->file_m != nullptr)
	{
		TIFFClose(this->file_m);
		this->file_m = nullptr;
	}
	if (this->file_y != nullptr)
	{
		TIFFClose(this->file_y);
		this->file_y = nullptr;
	}
	if (this->file_k != nullptr)
	{
		TIFFClose(this->file_k);
		this->file_k = nullptr;
	}
}
