#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

// Make all private members accessible for testing
#define private public

#include "../sepsource.hh"
#include "../sli/slilayer.hh"

const auto testFileDir =
    boost::dll::program_location().parent_path().parent_path() / "testfiles";

/** Test cases for sepsource.hh */

BOOST_AUTO_TEST_SUITE(SepSource_Tests)

BOOST_AUTO_TEST_CASE(create) {
  auto source = SepSource::create();
  BOOST_CHECK(source != nullptr);
}

BOOST_AUTO_TEST_CASE(parent_dir) {
  auto abctest = SepSource::findParentDir("abc/test/removed");
  BOOST_CHECK(abctest == "abc/test");
}

BOOST_AUTO_TEST_CASE(parse_sep) {
  SepFile file = SepSource::parseSep((testFileDir / "sep_cmyk.sep").string());
  BOOST_CHECK(file.files.size() == 4);

  for (const std::string &colour : {"C", "M", "Y", "K"}) {
    BOOST_CHECK(file.files[colour] == testFileDir / (colour + ".tif"));
  }

  BOOST_CHECK(file.width == 600);
  BOOST_CHECK(file.height == 400);
}

BOOST_AUTO_TEST_CASE(parse_sep_empty_line) {
  SepFile file =
      SepSource::parseSep((testFileDir / "sep_empty_line.sep").string());
  BOOST_CHECK(file.files.size() == 4);

  for (const std::string &colour : {"C", "M", "Y", "K"}) {
    BOOST_CHECK(file.files[colour] == testFileDir / (colour + ".tif"));
  }

  BOOST_CHECK(file.width == 600);
  BOOST_CHECK(file.height == 400);
}

BOOST_AUTO_TEST_CASE(parse_sep_empty_line_2) {
  SepFile file =
      SepSource::parseSep((testFileDir / "sep_empty_line_2.sep").string());
  BOOST_CHECK(file.files.size() == 4);

  for (const std::string &colour : {"C", "M", "Y", "K"}) {
    BOOST_CHECK(file.files[colour] == testFileDir / (colour + ".tif"));
  }

  BOOST_CHECK(file.width == 600);
  BOOST_CHECK(file.height == 400);
}

BOOST_AUTO_TEST_CASE(parse_sep_missing_channel) {
  // Y channel is not defined in this file
  SepFile file =
      SepSource::parseSep((testFileDir / "sep_missing_channel.sep").string());
  BOOST_CHECK(file.files.size() == 4);

  for (const std::string &colour : {"C", "M", "K"}) {
    BOOST_CHECK(file.files[colour] == testFileDir / (colour + ".tif"));
  }

  BOOST_CHECK(file.files["Y"].empty());
  BOOST_CHECK(file.width == 600);
  BOOST_CHECK(file.height == 400);
}

BOOST_AUTO_TEST_CASE(parse_sep_missing_channel_2) {
  // C channel is not defined in this file + empty line
  SepFile file =
      SepSource::parseSep((testFileDir / "sep_empty_line_3.sep").string());
  BOOST_CHECK(file.files.size() == 4);

  for (const std::string &colour : {"M", "Y", "K"}) {
    BOOST_CHECK(file.files[colour] == testFileDir / (colour + ".tif"));
  }

  BOOST_CHECK(file.files["C"].empty());
  BOOST_CHECK(file.width == 600);
  BOOST_CHECK(file.height == 400);
}

BOOST_AUTO_TEST_CASE(get_for_none) {
  uint16_t unit;
  float x_res, y_res;
  auto source = SepSource::create();

  source->getForOneChannel(nullptr, unit, x_res, y_res);

  BOOST_CHECK(unit == RESUNIT_NONE);
  BOOST_CHECK(std::abs(x_res - 1.0) < 1e-4);
  BOOST_CHECK(std::abs(y_res - 1.0) < 1e-4);
}

BOOST_AUTO_TEST_CASE(get_resolution_null) {
  uint16_t unit;
  float x_res, y_res;
  auto source = SepSource::create();

  source->channel_files["C"] = nullptr;
  source->channel_files["M"] = nullptr;
  source->channel_files["Y"] = nullptr;
  source->channel_files["K"] = nullptr;

  auto res = source->getResolution(unit, x_res, y_res);
  BOOST_CHECK(res == true);
}

BOOST_AUTO_TEST_CASE(get_transformation) {
  auto source = SepSource::create();

  source->channel_files["C"] = nullptr;
  source->channel_files["M"] = nullptr;
  source->channel_files["Y"] = nullptr;
  source->channel_files["K"] = nullptr;

  auto res = source->getTransform()->getAspectRatio();
  BOOST_CHECK(std::abs(res.x - 1.0) < 1e4);
  BOOST_CHECK(std::abs(res.y - 1.0) < 1e4);
}

BOOST_AUTO_TEST_CASE(set_data) {
  // Preparation
  auto source = SepSource::create();
  SepFile file = SepSource::parseSep((testFileDir / "sep_cmyk.sep").string());

  // Tested call
  source->setData(file);

  // Check properties, not the actual value, because we don't care if
  // setData copies the passed SepFile.
  BOOST_CHECK(source->sep_file.files.size() == 4);

  for (const std::string &colour : {"C", "M", "Y", "K"}) {
    BOOST_CHECK(source->sep_file.files[colour] ==
                testFileDir / (colour + ".tif"));
  }

  BOOST_CHECK(source->sep_file.width == 600);
  BOOST_CHECK(source->sep_file.height == 400);
}

BOOST_AUTO_TEST_CASE(open_files) {
  // Preparation
  auto source = SepSource::create();
  SepFile file = SepSource::parseSep((testFileDir / "sep_cmyk.sep").string());
  source->setData(file);

  // Tested call
  source->openFiles();

  // Check that all the CMYK files have been opened
  for (const std::string &colour : {"C", "M", "Y", "K"}) {
    BOOST_CHECK(source->channel_files[colour] != nullptr);
  }

  // Check that white ink and varnish are not opened
  BOOST_CHECK(source->white_ink == nullptr);
  BOOST_CHECK(source->sep_file.white_ink_choice == 0);
  BOOST_CHECK(source->varnish == nullptr);
}

BOOST_AUTO_TEST_CASE(open_files_twice) {
  // Preparation
  auto source = SepSource::create();
  SepFile file = SepSource::parseSep((testFileDir / "sep_cmyk.sep").string());
  source->setData(file);
  source->openFiles();

  // Save the TIFF pointers to make sure they don't change
  std::map<std::string, tiff *> files;
  for (const std::string &colour : {"C", "M", "Y", "K"}) {
    files[colour] = source->channel_files[colour];
  }

  // Tested call
  source->openFiles();

  // Check that all the CMYK files have not been opened again
  for (const std::string &colour : {"C", "M", "Y", "K"}) {
    BOOST_CHECK(source->channel_files[colour] == files[colour]);
  }
  BOOST_CHECK(source->sep_file.white_ink_choice == 0);
}

BOOST_AUTO_TEST_CASE(open_files_extra) {
  // Preparation
  auto source = SepSource::create();
  SepFile file = SepSource::parseSep((testFileDir / "sep_cmykv.sep").string());
  source->setData(file);

  // Tested call
  source->openFiles();

  // Check that all the CMYK files have been opened
  for (const std::string &colour : {"C", "M", "Y", "K"}) {
    BOOST_CHECK(source->channel_files[colour] != nullptr);
  }

  BOOST_CHECK(source->white_ink == nullptr);
  BOOST_CHECK(source->sep_file.white_ink_choice == 0);
  //check that varnish has been opened
  BOOST_CHECK(source->varnish != nullptr);
}

BOOST_AUTO_TEST_CASE(check_files) {
  // Preparation
  auto source = SepSource::create();
  SepFile file = SepSource::parseSep((testFileDir / "sep_cmykv.sep").string());
  source->setData(file);

  source->openFiles();
  BOOST_CHECK(source->sep_file.white_ink_choice == 0);
  
  // Tested call
  source->checkFiles();

  // Check that it did not crash -> did not throw warnings
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(apply_white_nowhite) {
  auto res = SepSource::applyWhiteInk(100, 100, 0);
  BOOST_CHECK(res == 100);
}

BOOST_AUTO_TEST_CASE(apply_white_subtract_1) {
  auto res = SepSource::applyWhiteInk(100, 100, 1);
  BOOST_CHECK(res == 0);
}

BOOST_AUTO_TEST_CASE(apply_white_subtract_2) {
  auto res = SepSource::applyWhiteInk(100, 101, 1);
  BOOST_CHECK(res == 1);
}

BOOST_AUTO_TEST_CASE(apply_white_multiply_1) {
  auto res = SepSource::applyWhiteInk(100, 100, 2);
  BOOST_CHECK(res == 61);
}

BOOST_AUTO_TEST_CASE(apply_white_multiply_2) {
  auto res = SepSource::applyWhiteInk(0, 100, 2);
  BOOST_CHECK(res == 100);
}

BOOST_AUTO_TEST_CASE(apply_white_no_effect_1) {
  auto res = SepSource::applyWhiteInk(0, 100, 0);
  BOOST_CHECK(res == 100);
}

BOOST_AUTO_TEST_CASE(tiff_wrapper_nullptr) {
  auto res = SepSource::TIFFReadScanline_(nullptr, nullptr, 1);
  BOOST_CHECK(res == -1);
}

BOOST_AUTO_TEST_CASE(tiff_wrapper_2) {
  auto file = TIFFOpen((testFileDir / "M_9.tif").string().c_str(), "r");
  auto lines = std::vector<uint8_t>(TIFFScanlineSize(file));
  int res = SepSource::TIFFReadScanline_(file, lines.data(), 1);
  BOOST_CHECK(res != -1);
}

BOOST_AUTO_TEST_CASE(fill_sli_empty) {
  SliLayer::Ptr sli = SliLayer::create("", "name", 0, 0);
  sli->height = 42;
  SepSource::Ptr sepSource = SepSource::create();
  sepSource->fillSliLayerMeta(sli);
  BOOST_CHECK(sli->height == 42);
}

BOOST_AUTO_TEST_CASE(closeIfNeeded_1) {
  auto file = TIFFOpen((testFileDir / "M_9.tif").string().c_str(), "r");
  BOOST_CHECK(file != nullptr);
  SepSource::closeIfNeeded(file);
  BOOST_CHECK_EQUAL(file, nullptr);
}

BOOST_AUTO_TEST_CASE(closeIfNeeded_2) {
  auto file = TIFFOpen(NULL, "r");
  BOOST_CHECK(file == nullptr);
  SepSource::closeIfNeeded(file);
  BOOST_CHECK_EQUAL(file, nullptr);
}

BOOST_AUTO_TEST_CASE(fill_sli_1) {
  SliLayer::Ptr sli =
      SliLayer::create((testFileDir / "sep_cmyk.sep").string(), "name", 0, 0);
  SepSource::Ptr sepSource = SepSource::create();
  sepSource->fillSliLayerMeta(sli);
  sepSource->fillSliLayerBitmap(sli);
  BOOST_CHECK(sli->width == 600);
  BOOST_CHECK(sli->height == 400);
  BOOST_CHECK(sli->spp == 4);
  BOOST_CHECK(sli->bps == 8);
  BOOST_CHECK(sli->bitmap != nullptr);
  BOOST_CHECK(std::abs(sli->xAspect - 1.0) < 1e-4);
  BOOST_CHECK(std::abs(sli->yAspect - 1.0) < 1e-4);
}

BOOST_AUTO_TEST_CASE(fill_no_tiles) {
  // Preparation
  std::vector<Tile::Ptr> tiles;
  auto source = SepSource::create();
  SepFile file = SepSource::parseSep((testFileDir / "sep_cmyk.sep").string());
  source->setData(file);
  source->openFiles();

  // Tested call
  source->fillTiles(0, 0, 4096, 0, tiles);

  // Checks that fillTiles did not crash
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
