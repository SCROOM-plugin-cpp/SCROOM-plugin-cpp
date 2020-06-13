#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

#include "sepsource.hh"

int ShowWarning(std::string message, GtkMessageType type_gtk = GTK_MESSAGE_WARNING)
{
	// We don't have a pointer to the parent window, so
	// we'll just supply nullptr..
	GtkWidget *dialog = gtk_message_dialog_new(
		nullptr, GTK_DIALOG_DESTROY_WITH_PARENT,
		type_gtk, GTK_BUTTONS_CLOSE, message.c_str());

	int k = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return k;
}

SepSource::SepSource() {}
SepSource::~SepSource() {}

SepSource::Ptr SepSource::create()
{
	// white_ink_type = 0;
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

		if (result.size() != 2 || result[1].empty() || (result[0].empty() && !result[1].empty()))
		{
			// Remember the warning and skip this line
			errors += "PANIC: One of the channels has not been provided correctly!\n";
			continue;
		}
		// std::cout << result[0] + "\t" << parent_dir + result[1] << std::endl;
		sep_file.files[result[0]] = parent_dir + result[1];
	}

	// Close the file
	file.close();

	sep_file.white_ink_choice = 0;

	// get user choice for white ink
	if (sep_file.files.count("W"))
	{
		auto white_choice = gtk_dialog_new_with_buttons("White Ink Effect",
														nullptr,
														GTK_DIALOG_DESTROY_WITH_PARENT,
														"Subtractive",
														GTK_RESPONSE_ACCEPT,
														"Multiplicative",
														GTK_RESPONSE_REJECT,
														NULL);
		// gtk_dialog_run(GTK_DIALOG(white_choice));
		if (gtk_dialog_run(GTK_DIALOG(white_choice)) == GTK_RESPONSE_ACCEPT)
			sep_file.white_ink_choice = 1;
		else
			sep_file.white_ink_choice = 2;

		gtk_widget_destroy(white_choice);
	}

	if (!errors.empty())
	{
		std::cerr << errors;
		ShowWarning(errors);
	}

	return sep_file;
}

void SepSource::getForOneChannel(struct tiff *channel, uint16_t &unit, float &x_resolution, float &y_resolution)
{
	if (TIFFGetField(channel, TIFFTAG_XRESOLUTION, &x_resolution) &&
		TIFFGetField(channel, TIFFTAG_YRESOLUTION, &y_resolution) &&
		TIFFGetField(channel, TIFFTAG_RESOLUTIONUNIT, &unit))
	{
		if (unit == RESUNIT_NONE)
			return;

		// Fix aspect ratio only
		float base = std::max(x_resolution, y_resolution);
		x_resolution /= base;
		y_resolution /= base;
		return;
	}

	// Defaults according to TIFF spec
	x_resolution = 1.0;
	y_resolution = 1.0;
	unit = RESUNIT_NONE;
}

bool SepSource::getResolution(uint16_t &unit, float &x_resolution, float &y_resolution)
{
	float channel_res_x, channel_res_y;
	uint16_t channel_res_unit;
	bool warning = false;

	// Use the values for the c channel as baseline
	this->getForOneChannel(this->channel_files[channels[0]], unit, x_resolution, y_resolution);

	for (auto channel : {this->channel_files[channels[1]], this->channel_files[channels[2]], this->channel_files[channels[3]]})
	{
		if (channel == nullptr)
			continue;

		this->getForOneChannel(channel, channel_res_unit, channel_res_x, channel_res_y);
		// check if the same as first values
		// if not, set status flag and continue
		warning = (channel_res_x != x_resolution) ||
				  (channel_res_y != y_resolution) ||
				  (channel_res_unit != unit);
	}

	return !warning;
}

TransformationData::Ptr SepSource::getTransform()
{
	uint16_t unit;
	float file_res_x, file_res_y;
	this->getResolution(unit, file_res_x, file_res_y);

	std::cout << "Resolution: " << 1 / file_res_x << " * " << 1 / file_res_y << "\n";

	TransformationData::Ptr data = TransformationData::create();
	data->setAspectRatio(1 / file_res_x, 1 / file_res_y);
	return data;
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
	source->setData(values);
	source->openFiles();

	uint16_t unit;
	source->getResolution(unit, sli->xAspect, sli->yAspect);

	auto temp = std::vector<byte>(row_width);
	for (int y = 0; y < sli->height; y++)
	{
		source->readCombinedScanline(temp, y);
		memcpy(sli->bitmap + y * row_width, temp.data(), row_width);
	}

	source->done();
}

void SepSource::setData(SepFile file)
{
	this->sep_file = file;
}

void SepSource::openFiles()
{
	for (auto c : channels)
	{
		if (channel_files[c] != nullptr)
		{
			printf("PANIC: %s file has already been initialized. Cannot open it again.\n", c.c_str());
			return;
		}
	}

	bool show_warning = false;

	// open CMYK channels
	for (auto c : channels)
	{
		channel_files[c] = TIFFOpen(this->sep_file.files[c].c_str(), "r");

		if (channel_files[c] == nullptr)
			show_warning = true;
	}

	// open white ink and varnish channels
	this->white_ink = TIFFOpen(this->sep_file.files["W"].c_str(), "r");
	this->varnish = TIFFOpen(this->sep_file.files["V"].c_str(), "r");
	show_warning = show_warning || (sep_file.files.count("W") && white_ink == nullptr) || (sep_file.files.count("V") && varnish == nullptr);
	if (show_warning)
	{
		printf("PANIC: One of the provided files is not valid, or could not be opened!");
		ShowWarning("PANIC: One of the provided files is not valid, or could not be opened!");
	}
}

int TIFFReadScanline_(tiff *file, void *buf, uint32 row, uint16 sample = 0)
{
	if (file == nullptr)
		return -1;
	return TIFFReadScanline(file, buf, row, sample);
}

void SepSource::readCombinedScanline(std::vector<byte> &out, size_t line_nr)
{
	// There are 4 channels in out, so the number of bytes an individual
	// channel has is one fourth of the output vector's size.
	size_t size = out.size() / 4;

	// Create buffers for the scanlines of the individual channels.
	std::vector<uint8_t> lines[nr_channels];
	for (size_t i = 0; i < nr_channels; i++)
	{
		lines[i] = std::vector<uint8_t>(size);
		TIFFReadScanline_(channel_files[channels[i]], lines[i].data(), line_nr);
	}

	auto w_line = std::vector<uint8_t>(size);
	auto v_line = std::vector<uint8_t>(size);
	TIFFReadScanline_(white_ink, w_line.data(), line_nr);
	TIFFReadScanline_(varnish, v_line.data(), line_nr);

	for (size_t i = 0; i < size; i++)
	{
		for (size_t j = 0; j < nr_channels; j++)
			out[4 * i + j] = applyWhiteInk(w_line[i], lines[j][i], this->sep_file.white_ink_choice);
	}
	// reset white_ink_type because it is static
	// SepSource::white_ink_type = 0;
}

uint8_t SepSource::applyWhiteInk(uint8_t white, uint8_t color, int type)
{
	if (type == 1) // 1 means subtractive model
		return white >= color ? 0 : color - white;
	else if (type == 2) // 2 means multiplicative model
		return white > 0 ? color / white : color;
	else // 0 means white ink is not present
		return color;
}

void SepSource::fillTiles(int startLine, int line_count, int tileWidth, int firstTile, std::vector<Tile::Ptr> &tiles)
{
	const size_t bpp = 4; // number of bytes per pixel
	const size_t start_line = static_cast<size_t>(startLine);
	const size_t first_tile = static_cast<size_t>(firstTile);
	const size_t tile_stride = static_cast<size_t>(tileWidth) * bpp;
	const size_t tile_count = tiles.size();

	// Open the TIFF files so we can read from them later.
	// I think this is a redundant call because openFiles() is already called in load function
	// this->openFiles();

	// Buffer for the scanline to be written into
	auto row = std::vector<byte>(bpp * this->sep_file.width);

	// Store the pointers to the beginning of the tiles in a
	// separate vector, so we can update it to point to the
	// start of the current row in the loop.
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

void SepSource::closeIfNeeded(struct tiff *&file)
{
	if (file == nullptr)
		return;

	TIFFClose(file);
	file = nullptr;
}

void SepSource::done()
{
	// Close all tiff files and reset pointers
	for (auto &x : this->channel_files)
	{
		std::cout << x.second << std::endl;
		closeIfNeeded(x.second);
	}
}
