#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

// Make all private members accessible for testing
#define private public

#include "../sli/slipresentation.hh"
#include <scroom/scroominterface.hh>

#define SLI_NOF_LAYERS 4

#include "constants.hh"

///////////////////////////////////////////////////////////////////////////////
// Helper functions

void dummyFunc(){};

SliPresentation::Ptr createPresentation() {
  SliPresentation::Ptr presentation = SliPresentation::create(nullptr);
  BOOST_REQUIRE(presentation);
  // Assign the callbacks to dummy functions to avoid exceptions
  presentation->source->enableInteractions = boost::bind(dummyFunc);
  presentation->source->disableInteractions = boost::bind(dummyFunc);

  return presentation;
}

void dummyRedraw(SliPresentation::Ptr presentation) {
  // Create dummy objects to call redraw() with
  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 100, 100);
  cairo_t *cr = cairo_create(surface);
  Scroom::Utils::Rectangle<double> rect(0.0, 0.0, 100.0, 100.0);

  boost::this_thread::sleep(boost::posix_time::millisec(500));
  // redraw() for all zoom levels from 5 to -2 and check whether cache has been
  // computed
  for (int zoom = 5; zoom > -3; zoom--) {
    presentation->redraw(nullptr, cr, rect, zoom);
    boost::this_thread::sleep(boost::posix_time::millisec(
        500)); // Very liberal, shouldn't fail beause of time
    BOOST_REQUIRE(presentation->source->rgbCache[std::min(0, zoom)]);
  }
  BOOST_REQUIRE(presentation);
}

///////////////////////////////////////////////////////////////////////////////
// Tests

BOOST_AUTO_TEST_SUITE(Sli_Tests)

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_tiffonly) {
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load((testFileDir / "sli_tiffonly.sli").string());
  BOOST_REQUIRE(presentation->getLayers().size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_seponly) {
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load((testFileDir / "sli_seponly.sli").string());
  BOOST_REQUIRE(presentation->getLayers().size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_septiffmixed) {
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load((testFileDir / "sli_septiffmixed.sli").string());
  BOOST_REQUIRE(presentation->getLayers().size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_scale) {
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load((testFileDir / "sli_scale.sli").string());
  BOOST_REQUIRE(presentation->getLayers().size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_xoffset) {
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load((testFileDir / "sli_xoffset.sli").string());
  BOOST_REQUIRE(presentation->getLayers().size() == SLI_NOF_LAYERS);
  dummyRedraw(presentation);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_varnish) {
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load((testFileDir / "sli_varnish.sli").string());
  std::cout << presentation->getLayers().size() << '\n';
  BOOST_REQUIRE(presentation->getLayers().size() == SLI_NOF_LAYERS);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_varnish_wrongpath) {
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load((testFileDir / "sli_varnish_wrongpath.sli").string());
  BOOST_REQUIRE(presentation->getLayers().size() == 0);
}

BOOST_AUTO_TEST_CASE(slipresentation_presentationinterface_inherited) {
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load(testFileDir.string());

  std::string nameStr = "testname";
  std::string valueStr;

  BOOST_REQUIRE(presentation->isPropertyDefined(nameStr) == false);
  BOOST_REQUIRE(presentation->getProperty(nameStr, valueStr) == false);
  BOOST_REQUIRE(valueStr == "");

  presentation->properties["testname"] = "testvalue";

  BOOST_REQUIRE(presentation->isPropertyDefined(nameStr) == true);
  BOOST_REQUIRE(presentation->getProperty(nameStr, valueStr) == true);
  BOOST_REQUIRE(valueStr == "testvalue");

  BOOST_REQUIRE(presentation->getTitle() == testFileDir.string());

  presentation.reset();
}

BOOST_AUTO_TEST_CASE(slipresentation_pipette_tool_multiple_colors) {
  SliPresentation::Ptr presentation = createPresentation();

  presentation->load((testFileDir / "sli_pipette.sli").string());
  BOOST_REQUIRE(presentation->getLayers().size() == 1);
  dummyRedraw(presentation);
  // Testing 4 CMYK pixels + rectangle larger than the canvas
  Scroom::Utils::Rectangle<int> rect1{0, 0, 3, 4};
  auto result = presentation->getPixelAverages(rect1);
  for (auto r : result)
    BOOST_REQUIRE(abs(r.second - 63.75) < 0.0001);
}

BOOST_AUTO_TEST_CASE(slipresentation_pipette_tool_one_color) {
  SliPresentation::Ptr presentation = createPresentation();

  presentation->load((testFileDir / "sli_pipette.sli").string());
  BOOST_REQUIRE(presentation->getLayers().size() == 1);
  dummyRedraw(presentation);
  // C
  Scroom::Utils::Rectangle<int> rect1{0, 0, 1, 1};
  auto result = presentation->getPixelAverages(rect1);
  BOOST_REQUIRE(abs(result[0].second - 255) < 0.0001);
  // M
  Scroom::Utils::Rectangle<int> rect2{1, 0, 1, 1};
  result = presentation->getPixelAverages(rect2);
  BOOST_REQUIRE(abs(result[1].second - 255) < 0.0001);
  // Y
  Scroom::Utils::Rectangle<int> rect3{0, 1, 1, 1};
  result = presentation->getPixelAverages(rect3);
  BOOST_REQUIRE(abs(result[2].second - 255) < 0.0001);
  // K
  Scroom::Utils::Rectangle<int> rect4{1, 1, 1, 1};
  result = presentation->getPixelAverages(rect4);
  BOOST_REQUIRE(abs(result[3].second - 255) < 0.0001);
}

BOOST_AUTO_TEST_CASE(slipresentation_pipette_tool_zero_area) {
  SliPresentation::Ptr presentation = createPresentation();
  presentation->load((testFileDir / "sli_pipette.sli").string());
  BOOST_REQUIRE(presentation->getLayers().size() == 1);
  dummyRedraw(presentation);

  Scroom::Utils::Rectangle<int> rect1{0, 0, 0, 0};
  auto result = presentation->getPixelAverages(rect1);
  BOOST_REQUIRE(result.empty());
}

BOOST_AUTO_TEST_SUITE_END()