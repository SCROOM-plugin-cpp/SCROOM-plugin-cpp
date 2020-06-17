#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

// Make all private members accessible for testing
#define private public

#include "../sli/slipresentation.hh"
#include <scroom/scroominterface.hh>

#define SLI_NOF_LAYERS 4

const std::string testFileDir = boost::dll::program_location().parent_path().parent_path().string() + "/testfiles/";

///////////////////////////////////////////////////////////////////////////////
// Helper functions

void dummyFunc() {};

SliPresentation::Ptr createPresentation()
{
  SliPresentation::Ptr presentation = SliPresentation::create(nullptr);
  BOOST_REQUIRE(presentation);
  // Assign the callbacks to dummy functions to avoid exceptions
  presentation->source->enableInteractions = boost::bind(dummyFunc);
  presentation->source->disableInteractions = boost::bind(dummyFunc);

  return presentation;
}

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

///////////////////////////////////////////////////////////////////////////////
// Tests

BOOST_AUTO_TEST_SUITE(Sli_Tests)

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_tiffonly)
{
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load(testFileDir + "sli_tiffonly.sli");
  BOOST_REQUIRE(presentation->source->layers.size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_seponly)
{
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load(testFileDir + "sli_seponly.sli");
  BOOST_REQUIRE(presentation->source->layers.size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_septiffmixed)
{
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load(testFileDir + "sli_septiffmixed.sli");
  BOOST_REQUIRE(presentation->source->layers.size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_scale)
{
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load(testFileDir + "sli_scale.sli");
  BOOST_REQUIRE(presentation->source->layers.size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_xoffset)
{
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load(testFileDir + "sli_xoffset.sli");
  BOOST_REQUIRE(presentation->source->layers.size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_SUITE_END()
