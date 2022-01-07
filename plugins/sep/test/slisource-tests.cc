#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include "../sli/slipresentation.hh"
#include <scroom/scroominterface.hh>

#define SLI_NOF_LAYERS 4

#include "testglobals.hh"

///////////////////////////////////////////////////////////////////////////////
// Helper functions

void dummyFunc1() {}

SliPresentation::Ptr createPresentation1() {
  SliPresentation::Ptr presentation = SliPresentation::create(nullptr);
  BOOST_REQUIRE(presentation);
  // Assign the callbacks to dummy functions to avoid exceptions
  presentation->source->enableInteractions = boost::bind(dummyFunc1);
  presentation->source->disableInteractions = boost::bind(dummyFunc1);

  return presentation;
}

void dummyRedraw1(SliPresentation::Ptr presentation) {
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
        1000)); // Very liberal, shouldn't fail beause of time
    BOOST_REQUIRE(presentation->source->rgbCache.at(std::min(0, zoom)));
  }
  BOOST_REQUIRE(presentation);
}

///////////////////////////////////////////////////////////////////////////////
// Tests

BOOST_AUTO_TEST_SUITE(Sli_Tests)

BOOST_AUTO_TEST_CASE(slisource_clearbottomsurface_all_toggled) {
  SliPresentation::Ptr presentation = createPresentation1();

  presentation->load(TestFiles::getPathToFile("sli_tiffonly.sli"));
  dummyRedraw1(presentation);
  presentation->source->toggled = boost::dynamic_bitset<>{SLI_NOF_LAYERS}.set();
  presentation->source->clearBottomSurface();
  int total_height = presentation->source->total_height;
  int total_width = presentation->source->total_width;
  bool allZero = true;
  auto surface = presentation->source->rgbCache.at(0)->getBitmap();
  for (int i = 0; i < total_height * total_width * 4; i++) {
    if (surface[i] != 0) {
      allZero = false;
      break;
    }
  }
  BOOST_REQUIRE(allZero == true);
}

BOOST_AUTO_TEST_CASE(slisource_clearbottomsurface_none_toggled) {
  SliPresentation::Ptr presentation1 = createPresentation1();
  SliPresentation::Ptr presentation2 = createPresentation1();

  presentation1->load(TestFiles::getPathToFile("sli_tiffonly.sli"));
  presentation2->load(TestFiles::getPathToFile("sli_tiffonly.sli"));
  dummyRedraw1(presentation1);
  dummyRedraw1(presentation2);
  presentation1->source->toggled = boost::dynamic_bitset<>{SLI_NOF_LAYERS};
  presentation1->source->clearBottomSurface();
  int total_height = presentation1->source->total_height;
  int total_width = presentation1->source->total_width;
  auto surface1 = presentation1->source->rgbCache.at(0)->getBitmap();
  auto surface2 = presentation2->source->rgbCache.at(0)->getBitmap();
  bool bothEqual = true;
  for (int i = 0; i < total_height * total_width * 4; i++) {
    if (surface1[i] != surface2[i]) {
      bothEqual = false;
      break;
    }
  }
  BOOST_REQUIRE(bothEqual == true);
}

BOOST_AUTO_TEST_CASE(slisource_clearbottomsurface_some_toggled) {
  SliPresentation::Ptr presentation1 = createPresentation1();
  SliPresentation::Ptr presentation2 = createPresentation1();

  presentation1->load(TestFiles::getPathToFile("sli_tiffonly.sli"));
  presentation2->load(TestFiles::getPathToFile("sli_tiffonly.sli"));
  dummyRedraw1(presentation1);
  dummyRedraw1(presentation2);
  presentation1->source->toggled =
      boost::dynamic_bitset<>{SLI_NOF_LAYERS}.set(0);
  presentation1->source->clearBottomSurface();
  int height = presentation1->source->layers[0]->height;
  int width = presentation1->source->layers[0]->width;
  int total_height = presentation1->source->total_height;
  int total_width = presentation1->source->total_width;
  auto surface1 = presentation1->source->rgbCache.at(0)->getBitmap();
  auto surface2 = presentation2->source->rgbCache.at(0)->getBitmap();

  bool someZero = true;
  for (int i = 0; i < height * width * 4; i++) {
    if (surface1[i] != 0) {
      someZero = false;
      break;
    }
  }
  BOOST_REQUIRE(someZero == true);

  bool bothEqual = true;
  for (int i = height * width * 4; i < total_height * total_width * 4; i++) {
    if (surface1[i] != surface2[i]) {
      bothEqual = false;
      break;
    }
  }
  BOOST_REQUIRE(bothEqual == true);
}

// tinycmyk.tif (cmyk) = [(255,0,0,0),(0,255,0,0),(0,0,255,0),(0,0,0,255)]
// tinycmyk.tif (bgra) =
//              [(255,255,0,255),(255,0,255,255),(0,255,255,255),(0,0,0,255)]
BOOST_AUTO_TEST_CASE(slisource_computergb_yoffset) {
  SliPresentation::Ptr presentation = createPresentation1();
  presentation->load(TestFiles::getPathToFile("sli_tinycmyk.sli"));
  dummyRedraw1(presentation);
  auto surface = presentation->source->getSurface(0)->getBitmap();
  presentation->source->computeRgb();
  // bgra conversion of tinycmyk.tif
  uint8_t tinycmyk[] = {255, 255, 0,   255, 255, 0, 255, 255,
                        0,   255, 255, 255, 0,   0, 0,   255};

  for (int i = 0; i < 2 * 2 * 4; i++) {
    BOOST_REQUIRE(surface[i] == tinycmyk[i]);
  }
}

BOOST_AUTO_TEST_CASE(slisource_computergb_xoffset) {
  SliPresentation::Ptr presentation = createPresentation1();
  presentation->load(TestFiles::getPathToFile("sli_tinycmyk_xoffset.sli"));
  dummyRedraw1(presentation);
  auto surface = presentation->source->rgbCache.at(0)->getBitmap();
  presentation->source->computeRgb();
  // bgra conversion of tinycmyk.tif
  uint8_t tinycmyk[] = {0, 0, 0, 0, 255, 255, 0,   255, 255, 0, 255, 255,
                        0, 0, 0, 0, 0,   255, 255, 255, 0,   0, 0,   255};

  for (int i = 0; i < 2 * 3 * 4; i++) {
    BOOST_REQUIRE(surface[i] == tinycmyk[i]);
  }
}

BOOST_AUTO_TEST_SUITE_END()