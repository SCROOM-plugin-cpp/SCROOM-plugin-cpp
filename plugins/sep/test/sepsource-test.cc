#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>

// Make all private members accessible for testing
#define private public

#include "../sepsource.hh"
#include "../sli/slilayer.hh"

const auto testFileDir = boost::dll::program_location().parent_path().parent_path() / boost::filesystem::path("testfiles");

BOOST_AUTO_TEST_SUITE(Sep_Tests)

BOOST_AUTO_TEST_CASE(create) {
    auto source = SepSource::create();
    BOOST_CHECK(source != nullptr);
}

BOOST_AUTO_TEST_CASE(parent_dir) {
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(parse_sep) {
    SepFile file = SepSource::parseSep((testFileDir / "sep_cmyk.sep").string());
    printf("%ld\n", file.files.size());
    BOOST_CHECK(file.files.size() == 4);

    for (const std::string& colour : {"C", "M", "Y", "K"}) {
        BOOST_CHECK(file.files[colour] == testFileDir / (colour + ".tif"));
    }

    BOOST_CHECK(file.width == 600);
    BOOST_CHECK(file.height == 400);
    BOOST_CHECK(file.white_ink_choice == 0);
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

    for (const std::string& colour : {"C", "M", "Y", "K"}) {
        BOOST_CHECK(source->sep_file.files[colour] == testFileDir / (colour + ".tif"));
    }

    BOOST_CHECK(source->sep_file.width == 600);
    BOOST_CHECK(source->sep_file.height == 400);
    BOOST_CHECK(source->sep_file.white_ink_choice == 0);
}

BOOST_AUTO_TEST_CASE(open_files) {
    // Preparation
    auto source = SepSource::create();
    SepFile file = SepSource::parseSep((testFileDir / "sep_cmyk.sep").string());
    source->setData(file);

    // Tested call
    source->openFiles();

    // Check that all the CMYK files have been opened
    for (const std::string& colour : {"C", "M", "Y", "K"}) {
        BOOST_CHECK(source->channel_files[colour] != nullptr);
    }

    // Check that white ink and varnish are not opened
    BOOST_CHECK(source->white_ink == nullptr);
    BOOST_CHECK(source->varnish == nullptr);
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
    BOOST_CHECK(res == 60);
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
    auto res = SepSource::TIFFReadScanline_(nullptr, nullptr, 1);
    BOOST_CHECK(res == -1);
}

BOOST_AUTO_TEST_CASE(fill_sli_1) {
    SliLayer::Ptr sli = SliLayer::create("/home/ubuntu/Documents/swathStack_4pass_ONYX_Quality_Evaluation/swath_009.sep", "swath_009.sep", 0, 0);
    SepSource::fillSliLayer(sli);
    BOOST_CHECK_EQUAL(sli->width, 20704);
    BOOST_CHECK_EQUAL(sli->height, 1024);
    BOOST_CHECK_EQUAL(sli->spp, 4);
    BOOST_CHECK_EQUAL(sli->bps, 8);
    BOOST_CHECK(sli->bitmap != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
