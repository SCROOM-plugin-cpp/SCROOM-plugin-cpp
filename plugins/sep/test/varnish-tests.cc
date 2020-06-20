#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

// Make all private members accessible for testing
#define private public

#include "../varnish/varnish.hh"
#include "../varnish/varnish-helpers.hh"
#include <scroom/scroominterface.hh>

const std::string testFileDir = boost::dll::program_location().parent_path().parent_path().string() + "/testfiles/";

///////////////////////////////////////////////////////////////////////////////
// Helper functions

/*
void dummyFunc() {};

void dummyRedraw(SliPresentation::Ptr presentation)
{
  // Create dummy objects to call redraw() with
  cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 100, 100);
  cairo_t* cr = cairo_create(surface);
  Scroom::Utils::Rectangle<double> rect(5.0, 5.0, 100.0, 100.0);

  // redraw() for all zoom levels from 5 to -2 and check whether cache has been computed
  for (int zoom = 5; zoom > -3; zoom--)
  {
    presentation->redraw(nullptr, cr, rect, zoom);
    boost::this_thread::sleep(boost::posix_time::millisec(500)); // Very liberal, shouldn't fail beause of time
    BOOST_REQUIRE(presentation->source->rgbCache[std::min(0, zoom)]);
  }
  BOOST_REQUIRE(presentation);
}
*/

///////////////////////////////////////////////////////////////////////////////
// Tests

BOOST_AUTO_TEST_SUITE(varnish_tests)

BOOST_AUTO_TEST_CASE(varnish_load_valid_tiff)
{
  SliLayer::Ptr test_varnishLayer = SliLayer::create(testFileDir+"v_valid.tif", "SomeCoolTitle", 0, 0);
  fillVarnishOverlay(test_varnishLayer);
  Varnish::Ptr test_varnish = Varnish::create(test_varnishLayer);
  // Properties set correctly?
  BOOST_REQUIRE(test_varnish->layer->name == "SomeCoolTitle");
  BOOST_REQUIRE(test_varnish->layer->filepath == testFileDir+"v_valid.tif");
  BOOST_REQUIRE(test_varnish->layer->width == 20);
  BOOST_REQUIRE(test_varnish->layer->height == 20);
  BOOST_REQUIRE(test_varnish->layer->xoffset == 0);
  BOOST_REQUIRE(test_varnish->layer->yoffset == 0);
  BOOST_REQUIRE(test_varnish->layer->xAspect == 1);
  BOOST_REQUIRE(test_varnish->layer->yAspect == 1);
  BOOST_REQUIRE(test_varnish->inverted == false);
  // Valid cairo surface?
  BOOST_REQUIRE(test_varnish->surface);
}

BOOST_AUTO_TEST_CASE(varnish_load_valid_tiff_centimeter)
{
  SliLayer::Ptr test_varnishLayer = SliLayer::create(testFileDir+"v_valid_centimeter.tif", "SomeCoolTitle", 0, 0);
  fillVarnishOverlay(test_varnishLayer);
  Varnish::Ptr test_varnish = Varnish::create(test_varnishLayer);
  // Properties set correctly?
  BOOST_REQUIRE(test_varnish->layer->name == "SomeCoolTitle");
  BOOST_REQUIRE(test_varnish->layer->filepath == testFileDir+"v_valid_centimeter.tif");
  BOOST_REQUIRE(test_varnish->layer->width == 20);
  BOOST_REQUIRE(test_varnish->layer->height == 20);
  BOOST_REQUIRE(test_varnish->layer->xoffset == 0);
  BOOST_REQUIRE(test_varnish->layer->yoffset == 0);
  BOOST_REQUIRE(test_varnish->layer->xAspect == 1);
  BOOST_REQUIRE(test_varnish->layer->yAspect == 1);
  BOOST_REQUIRE(test_varnish->inverted == false);
  // Valid cairo surface?
  BOOST_REQUIRE(test_varnish->surface);
}

BOOST_AUTO_TEST_CASE(varnish_load_valid_tiff_no_spp_tag)
{
  // This test file does not have a SamplePerPixel tag set, but should default to 1 spp
  SliLayer::Ptr test_varnishLayer = SliLayer::create(testFileDir+"v_valid_no_spp_tag.tif", "SomeCoolTitle", 0, 0);
  fillVarnishOverlay(test_varnishLayer);
  Varnish::Ptr test_varnish = Varnish::create(test_varnishLayer);
  // Properties set correctly?
  BOOST_REQUIRE(test_varnish->layer->name == "SomeCoolTitle");
  BOOST_REQUIRE(test_varnish->layer->filepath == testFileDir+"v_valid_no_spp_tag.tif");
  BOOST_REQUIRE(test_varnish->layer->width == 20);
  BOOST_REQUIRE(test_varnish->layer->height == 20);
  BOOST_REQUIRE(test_varnish->layer->xoffset == 0);
  BOOST_REQUIRE(test_varnish->layer->yoffset == 0);
  BOOST_REQUIRE(test_varnish->layer->xAspect == 1);
  BOOST_REQUIRE(test_varnish->layer->yAspect == 1);
  BOOST_REQUIRE(test_varnish->inverted == false);
  // Valid cairo surface?
  BOOST_REQUIRE(test_varnish->surface);
}

BOOST_AUTO_TEST_CASE(varnish_load_invalid_tiff)
{
  SliLayer::Ptr test_varnishLayer = SliLayer::create(testFileDir+"v_invalidrgb.tif", "Another Title", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!fillVarnishOverlay(test_varnishLayer));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "Another Title");
  BOOST_REQUIRE(test_varnishLayer->filepath == testFileDir+"v_invalidrgb.tif");
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_CASE(varnish_load_zero_res_tiff)
{
  // This test file is missinga width tag
  SliLayer::Ptr test_varnishLayer = SliLayer::create(testFileDir+"v_invalid_no_width.tif", "Another Title", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!fillVarnishOverlay(test_varnishLayer));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "Another Title");
  BOOST_REQUIRE(test_varnishLayer->filepath == testFileDir+"v_invalid_no_width.tif");
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_CASE(varnish_load_corrupted_tiff)
{
  // This test files magic number and file data was randomly edited.
  SliLayer::Ptr test_varnishLayer = SliLayer::create(testFileDir+"v_corrupted.tif", "Another Title", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!fillVarnishOverlay(test_varnishLayer));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "Another Title");
  BOOST_REQUIRE(test_varnishLayer->filepath == testFileDir+"v_corrupted.tif");
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_CASE(varnish_load_nonexistent_tiff)
{
  SliLayer::Ptr test_varnishLayer = SliLayer::create(testFileDir+"v_nonexistent.tif", "This file doesn't exist", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!fillVarnishOverlay(test_varnishLayer));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "This file doesn't exist");
  BOOST_REQUIRE(test_varnishLayer->filepath == testFileDir+"v_nonexistent.tif");
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_CASE(varnish_load_1bps_tiff)
{
  // This test file has 1 bps, which is not supported
  SliLayer::Ptr test_varnishLayer = SliLayer::create(testFileDir+"v_invalid1bps.tif", "TitleGoesHere", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!fillVarnishOverlay(test_varnishLayer));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "TitleGoesHere");
  BOOST_REQUIRE(test_varnishLayer->filepath == testFileDir+"v_invalid1bps.tif");
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}
BOOST_AUTO_TEST_CASE(varnish_load_invalid_no_bps_tiff)
{
  // This test file doesn't have a bps tag, thus will default to 1 bps. 1bps is not supported for varnish
  SliLayer::Ptr test_varnishLayer = SliLayer::create(testFileDir+"v_invalid_no_bps_tag.tif", "TitleGoesHere", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!fillVarnishOverlay(test_varnishLayer));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "TitleGoesHere");
  BOOST_REQUIRE(test_varnishLayer->filepath == testFileDir+"v_invalid_no_bps_tag.tif");
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
