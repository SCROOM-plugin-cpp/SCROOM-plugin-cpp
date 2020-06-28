#include "sepsource.hh"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <fstream>
#include <iostream>

#include "sep-helpers.hh"

SepSource::SepSource() {}
SepSource::~SepSource() {}

SepSource::Ptr SepSource::create() { return Ptr(new SepSource()); }

boost::filesystem::path SepSource::findParentDir(const std::string &file_path) {
  return boost::filesystem::path(file_path).parent_path();
}

/**
 * Parses the content of a given SEP file.
 *
 * When the width or height are not properly specified or one of the channel
 * names is empty or one of the channel lines does not follow the specification,
 * a warning dialog is shown.
 */
SepFile SepSource::parseSep(const std::string &file_name) {
  std::ifstream file(file_name);
  std::string line;

  SepFile sep_file;
  std::string warnings = "";
  const auto parent_dir = SepSource::findParentDir(file_name);

  // Read the first two lines of the file seperately, since they follow
  // a slightly different format (i.e. don't have a colon) and are to be
  // interpreted as integers defining the width and height of the image.
  try {
    std::getline(file, line);
    sep_file.width = std::stoul(line);
    std::getline(file, line);
    sep_file.height = std::stoul(line);
  } catch (const std::exception &e) {
    sep_file.width = 0;
    sep_file.height = 0;
    warnings += "WARNING: Width or height have not been provided correctly!\n";
  }

  // Make sure the required channels exist (albeit with empty paths).
  // Channels with empty paths are ignored during loading.
  sep_file.files = {{"C", ""}, {"M", ""}, {"Y", ""}, {"K", ""}};

  // Read lines of the file
  while (std::getline(file, line)) {
    std::vector<std::string> result;
    boost::split(result, line, boost::is_any_of(":"));

    if (result.size() != 2) {
      // Malformed line: Wrong number of colons.
      continue;
    }

    boost::algorithm::trim(result[0]);
    boost::algorithm::trim(result[1]);

    if (result[0].empty() || result[1].empty()) {
      // Malformed line: nothing before or after the colon.
      continue;
    }

    if (result[0] != "C" && result[0] != "M" && result[0] != "Y" &&
        result[0] != "K" && result[0] != "V" && result[0] != "W") {
      // Unsupported channel
      warnings += "WARNING: The .sep file defines an unknown channel (not C, "
                  "M, Y, K, V or W)!\n";
      continue;
    }

    // store the full file path to each file
    sep_file.files[result[0]] = parent_dir / result[1];
  }

  // We no longer need to read from the file, so we
  // can safely close it.
  file.close();

  // show errors if there are any
  if (!warnings.empty()) {
    std::cerr << warnings;
    ShowWarning(warnings);
  }

  return sep_file;
}

void SepSource::getForOneChannel(struct tiff *channel, uint16_t &unit,
                                 float &x_resolution, float &y_resolution) {
  if (channel != nullptr &&
      TIFFGetField(channel, TIFFTAG_XRESOLUTION, &x_resolution) &&
      TIFFGetField(channel, TIFFTAG_YRESOLUTION, &y_resolution) &&
      TIFFGetField(channel, TIFFTAG_RESOLUTIONUNIT, &unit)) {
    if (unit == RESUNIT_NONE) {
      return;
    }

    // Reduce the x and y resolution so they are both at most 1 and
    // leave the unit unchanged.
    float base = std::max(x_resolution, y_resolution);
    x_resolution /= base;
    y_resolution /= base;
    return;
  }

  // No resolution was provided in the input file, so set the to their
  // default values according to the TIFF specification.
  x_resolution = 1.0;
  y_resolution = 1.0;
  unit = RESUNIT_NONE;
}

bool SepSource::getResolution(uint16_t &unit, float &x_resolution,
                              float &y_resolution) {
  float channel_res_x, channel_res_y;
  uint16_t channel_res_unit;
  bool warning = false;

  // Use the values for the c channel as baseline
  getForOneChannel(channel_files[channels[0]], unit, x_resolution,
                   y_resolution);

  for (auto channel : {channel_files[channels[1]], channel_files[channels[2]],
                       channel_files[channels[3]]}) {
    if (channel == nullptr) {
      continue;
    }

    getForOneChannel(channel, channel_res_unit, channel_res_x, channel_res_y);
    // check if the same as first values
    // if not, set status flag and continue
    warning |= std::abs(channel_res_x - x_resolution) > 1e-3 ||
               std::abs(channel_res_y - y_resolution) > 1e-3 ||
               channel_res_unit != unit;
  }

  return !warning;
}

TransformationData::Ptr SepSource::getTransform() {
  uint16_t unit;
  float file_res_x, file_res_y;
  getResolution(unit, file_res_x, file_res_y);

  TransformationData::Ptr data = TransformationData::create();
  data->setAspectRatio(1 / file_res_x, 1 / file_res_y);
  return data;
}

void SepSource::fillSliLayerMeta(SliLayer::Ptr sli) {
  if (sli->filepath.empty()) {
    return;
  }

  const SepFile values = parseSep(sli->filepath);

  sli->height = values.height;
  sli->width = values.width;
  sli->spp = 4;
  sli->bps = 8;

  setData(values);
  openFiles();
  checkFiles();
}

void SepSource::fillSliLayerBitmap(SliLayer::Ptr sli) {
  uint16_t unit;
  getResolution(unit, sli->xAspect, sli->yAspect);

  const int row_width = sli->width * 4; // 4 bytes per pixel
  sli->bitmap = new uint8_t[sli->height * row_width];

  auto temp = std::vector<byte>(row_width);
  for (int y = 0; y < sli->height; y++) {
    readCombinedScanline(temp, y);
    memcpy(sli->bitmap + y * row_width, temp.data(), row_width);
  }
}

void SepSource::setData(SepFile file) { sep_file = file; }

void SepSource::openFiles() {
  for (auto c : channels) {
    if (channel_files[c] != nullptr) {
      printf("WARNING: %s file has already been initialized. Cannot open it "
             "again.\n",
             c.c_str());
      return;
    }
  }

  bool show_warning = false;

  // open CMYK channels
  for (auto c : channels) {
    channel_files[c] = TIFFOpen(sep_file.files[c].string().c_str(), "r");

    // Don't show a warning when the file path is empty. This means
    // that the file was not specified, and the customer requested
    // there not to be a warning in that case.
    show_warning |= !sep_file.files[c].empty() && channel_files[c] == nullptr;
  }

  sep_file.white_ink_choice = 0;

  // open white ink channel
  if (sep_file.files.count("W") == 1) {
    white_ink = TIFFOpen(sep_file.files["W"].string().c_str(), "r");
    show_warning |= white_ink == nullptr;

    // Ask the user how to interpret the white ink values
    if (white_ink != nullptr) {
      auto choice_dialog = gtk_dialog_new_with_buttons(
          "White Ink Effect", nullptr, GTK_DIALOG_DESTROY_WITH_PARENT,
          "Subtractive", GTK_RESPONSE_ACCEPT, "Multiplicative",
          GTK_RESPONSE_REJECT, nullptr);

      auto choice = gtk_dialog_run(GTK_DIALOG(choice_dialog));
      sep_file.white_ink_choice = choice == GTK_RESPONSE_ACCEPT ? 1 : 2;

      gtk_widget_destroy(choice_dialog);
    }
  }

  // open varnish channel
  if (sep_file.files.count("V") == 1) {
    SliLayer::Ptr varnishLayer =
        SliLayer::create(sep_file.files["V"].string(), "Varnish", 0, 0);
    if (varnishLayer->fillMetaFromTiff(8, 1)) {
      varnish = Varnish::create(varnishLayer);
    } else {
      show_warning = true;
    }
  }

  if (show_warning) {
    printf("PANIC: One of the provided files is not valid, or could not be "
           "opened!\n");
    ShowWarning("PANIC: One of the provided files is not valid, or could not "
                "be opened!");
  }
}

void SepSource::checkFiles() {
  uint16_t spp, bps;
  std::string warning = "";

  // check CMYK
  for (auto c : channels) {
    if (channel_files[c] != nullptr &&
        TIFFGetField(channel_files[c], TIFFTAG_SAMPLESPERPIXEL, &spp) == 1 &&
        spp != 1) {
      warning += "ERROR: Samples per pixel is not 1!\n";
    }
    if (channel_files[c] != nullptr &&
        TIFFGetField(channel_files[c], TIFFTAG_BITSPERSAMPLE, &bps) == 1 &&
        bps != 8) {
      warning += "ERROR: Bits per sample is not 8!\n";
    }
  }

  // check white ink
  if (white_ink != nullptr) {
    if (TIFFGetField(white_ink, TIFFTAG_SAMPLESPERPIXEL, &spp) == 1 &&
        spp != 1) {
      warning += "ERROR: White ink samples per pixel is not 1!\n";
    }
    if (TIFFGetField(white_ink, TIFFTAG_BITSPERSAMPLE, &bps) == 1 && bps != 8) {
      warning += "ERROR: White ink bits per sample is not 8!\n";
    }
  }

  if (!warning.empty()) {
    ShowWarning(warning);
  }
}

int SepSource::TIFFReadScanline_(tiff *file, void *buf, uint32 row,
                                 uint16 sample) {
  return file == nullptr ? -1 : TIFFReadScanline(file, buf, row, sample);
}

void SepSource::readCombinedScanline(std::vector<byte> &out, size_t line_nr) {
  // There are 4 channels in out, so the number of bytes an individual
  // channel has is one fourth of the output vector's size.
  size_t size = out.size() / 4;

  // Create buffers for the scanlines of the individual channels.
  std::vector<uint8_t> lines[nr_channels];
  for (size_t i = 0; i < nr_channels; i++) {
    lines[i] = std::vector<uint8_t>(size);
    TIFFReadScanline_(channel_files[channels[i]], lines[i].data(), line_nr);
  }

  auto w_line = std::vector<uint8_t>(size);
  TIFFReadScanline_(white_ink, w_line.data(), line_nr);

  for (size_t i = 0; i < size; i++) {
    for (size_t j = 0; j < nr_channels; j++) {
      out[nr_channels * i + j] = SepSource::applyWhiteInk(
          w_line[i], lines[j][i], sep_file.white_ink_choice);
    }
  }
}

uint8_t SepSource::applyWhiteInk(uint8_t white, uint8_t color, int type) {
  if (type == 1) // 1 means subtractive model
    return white >= color ? 0 : color - white;
  else if (type == 2)                   // 2 means multiplicative model
    return color - color * white / 255; // 255 is the maximum white value
  else                                  // 0 means white ink is not present
    return color;
}

void SepSource::fillTiles(int startLine, int line_count, int tileWidth,
                          int firstTile, std::vector<Tile::Ptr> &tiles) {
  const size_t bpp = 4; // number of bytes per pixel
  const size_t start_line = static_cast<size_t>(startLine);
  const size_t first_tile = static_cast<size_t>(firstTile);
  const size_t tile_stride = static_cast<size_t>(tileWidth) * bpp;
  const size_t tile_count = tiles.size();

  // Buffer for the scanline to be written into
  auto row = std::vector<byte>(bpp * sep_file.width);

  // Store the pointers to the beginning of the tiles in a
  // separate vector, so we can update it to point to the
  // start of the current row in the loop.
  auto tile_data = std::vector<byte *>(tile_count);
  for (size_t tile = 0; tile < tile_count; tile++) {
    tile_data[tile] = tiles[tile]->data.get();
  }

  // The number of bytes that are in the full tiles of the image
  const size_t accounted_width = (first_tile + tile_count - 1) * tile_stride;

  // The number of remaining bytes
  const size_t remaining_width = bpp * sep_file.width - accounted_width;

  // This points to the beginning of the row (taking the starting tile
  // into account).
  const byte *horizontal_offset = row.data() + first_tile * tile_stride;

  for (size_t i = 0; i < static_cast<size_t>(line_count); i++) {
    readCombinedScanline(row, i + start_line);

    // The general case for completely filled tiles. The last tile
    // is the only tile that might not be completely filled, so that
    // case has a separate implementation below.
    for (size_t tile = 0; tile < tile_count - 1; tile++) {
      memcpy(tile_data[tile], horizontal_offset + tile * tile_stride,
             tile_stride);
      tile_data[tile] += tile_stride;
    }

    // Copy the data into the last tile. This tile might not be
    // completely filled, so that's why this case is not included
    // in the for loop.
    memcpy(tile_data[tile_count - 1], row.data() + accounted_width,
           remaining_width);
    tile_data[tile_count - 1] += tile_stride;
  }
}

void SepSource::closeIfNeeded(struct tiff *&file) {
  if (file == nullptr) {
    return;
  }
  TIFFClose(file);
  file = nullptr;
}

void SepSource::done() {
  // Close all tiff files and reset pointers
  for (auto &x : channel_files) {
    SepSource::closeIfNeeded(x.second);
  }
}
