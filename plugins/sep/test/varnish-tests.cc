#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include "../varnish/varnish.hh"
#include "testglobals.hh"
#include <scroom/scroominterface.hh>
#include <scroom/viewinterface.hh>

///////////////////////////////////////////////////////////////////////////////
// Helper object

class DummyViewInterface : public ViewInterface {

public:
  DummyViewInterface(){};
  ~DummyViewInterface(){};
  void invalidate(){};
  ProgressInterface::Ptr getProgressInterface() { return nullptr; }

  static ViewInterface::Ptr create() {
    return ViewInterface::Ptr(new DummyViewInterface());
  }

  void addSideWidget(std::string, GtkWidget *w) {
    require(Scroom::GtkHelpers::on_ui_thread());

    // Add the widget to a fresh GTK box instead.
    // This won't verify UI integrity, but it should
    // at least allow the code to run once in headless CI
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), w, true, true, 0);
  }
  void removeSideWidget(GtkWidget *) {
    require(Scroom::GtkHelpers::on_ui_thread());
  };
  void addToToolbar(GtkToolItem *) {
    require(Scroom::GtkHelpers::on_ui_thread());
  };
  void removeFromToolbar(GtkToolItem *) {
    require(Scroom::GtkHelpers::on_ui_thread());
  };
  void registerSelectionListener(SelectionListener::Ptr){};
  void registerPostRenderer(PostRenderer::Ptr){};
  void setStatusMessage(const std::string &) {
    require(Scroom::GtkHelpers::on_ui_thread());
  };
  boost::shared_ptr<PresentationInterface> getCurrentPresentation() {
    return nullptr;
  };
  void addToolButton(GtkToggleButton *, ToolStateListener::Ptr) {
    require(Scroom::GtkHelpers::on_ui_thread());
  };
};

void dummyFunction() {}

///////////////////////////////////////////////////////////////////////////////
// Tests for varnish loading

BOOST_AUTO_TEST_SUITE(varnish_tests)

BOOST_AUTO_TEST_CASE(varnish_load_valid_tiff) {
  SliLayer::Ptr test_varnishLayer = SliLayer::create(
      TestFiles::getPathToFile("v_valid.tif"), "SomeCoolTitle", 0, 0);
  test_varnishLayer->fillMetaFromTiff(8, 1);
  test_varnishLayer->fillBitmapFromTiff();
  Varnish::Ptr test_varnish = Varnish::create(test_varnishLayer);
  // Properties set correctly?
  BOOST_REQUIRE(test_varnish->layer->name == "SomeCoolTitle");
  BOOST_REQUIRE(test_varnish->layer->filepath ==
                TestFiles::getPathToFile("v_valid.tif"));
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

BOOST_AUTO_TEST_CASE(varnish_load_valid_tiff_centimeter) {
  SliLayer::Ptr test_varnishLayer =
      SliLayer::create(TestFiles::getPathToFile("v_valid_centimeter.tif"),
                       "SomeCoolTitle", 0, 0);
  test_varnishLayer->fillMetaFromTiff(8, 1);
  test_varnishLayer->fillBitmapFromTiff();
  Varnish::Ptr test_varnish = Varnish::create(test_varnishLayer);
  // Properties set correctly?
  BOOST_REQUIRE(test_varnish->layer->name == "SomeCoolTitle");
  BOOST_REQUIRE(test_varnish->layer->filepath ==
                TestFiles::getPathToFile("v_valid_centimeter.tif"));
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

BOOST_AUTO_TEST_CASE(varnish_load_valid_tiff_no_spp_tag) {
  // This test file does not have a SamplePerPixel tag set, but should default
  // to 1 spp
  SliLayer::Ptr test_varnishLayer =
      SliLayer::create(TestFiles::getPathToFile("v_valid_no_spp_tag.tif"),
                       "SomeCoolTitle", 0, 0);
  test_varnishLayer->fillMetaFromTiff(8, 1);
  test_varnishLayer->fillBitmapFromTiff();
  Varnish::Ptr test_varnish = Varnish::create(test_varnishLayer);
  // Properties set correctly?
  BOOST_REQUIRE(test_varnish->layer->name == "SomeCoolTitle");
  BOOST_REQUIRE(test_varnish->layer->filepath ==
                TestFiles::getPathToFile("v_valid_no_spp_tag.tif"));
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

BOOST_AUTO_TEST_CASE(varnish_load_invalid_tiff) {
  SliLayer::Ptr test_varnishLayer = SliLayer::create(
      TestFiles::getPathToFile("v_invalidrgb.tif"), "Another Title", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!test_varnishLayer->fillMetaFromTiff(8, 1));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "Another Title");
  BOOST_REQUIRE(test_varnishLayer->filepath ==
                TestFiles::getPathToFile("v_invalidrgb.tif"));
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_CASE(varnish_load_zero_res_tiff) {
  // This test file is missinga width tag
  SliLayer::Ptr test_varnishLayer =
      SliLayer::create(TestFiles::getPathToFile("v_invalid_no_width.tif"),
                       "Another Title", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!test_varnishLayer->fillMetaFromTiff(8, 1));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "Another Title");
  BOOST_REQUIRE(test_varnishLayer->filepath ==
                TestFiles::getPathToFile("v_invalid_no_width.tif"));
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_CASE(varnish_load_corrupted_tiff) {
  // This test files magic number and file data was randomly edited.
  SliLayer::Ptr test_varnishLayer = SliLayer::create(
      TestFiles::getPathToFile("v_corrupted.tif"), "Another Title", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!test_varnishLayer->fillMetaFromTiff(8, 1));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "Another Title");
  BOOST_REQUIRE(test_varnishLayer->filepath ==
                TestFiles::getPathToFile("v_corrupted.tif"));
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_CASE(varnish_load_nonexistent_tiff) {
  SliLayer::Ptr test_varnishLayer =
      SliLayer::create(TestFiles::getPathToFile("v_nonexistent.tif"),
                       "This file doesn't exist", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!test_varnishLayer->fillMetaFromTiff(8, 1));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "This file doesn't exist");
  BOOST_REQUIRE(test_varnishLayer->filepath ==
                TestFiles::getPathToFile("v_nonexistent.tif"));
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_CASE(varnish_load_1bps_tiff) {
  // This test file has 1 bps, which is not supported
  SliLayer::Ptr test_varnishLayer = SliLayer::create(
      TestFiles::getPathToFile("v_invalid1bps.tif"), "TitleGoesHere", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!test_varnishLayer->fillMetaFromTiff(8, 1));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "TitleGoesHere");
  BOOST_REQUIRE(test_varnishLayer->filepath ==
                TestFiles::getPathToFile("v_invalid1bps.tif"));
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}
BOOST_AUTO_TEST_CASE(varnish_load_invalid_no_bps_tiff) {
  // This test file doesn't have a bps tag, thus will default to 1 bps. 1bps is
  // not supported for varnish
  SliLayer::Ptr test_varnishLayer =
      SliLayer::create(TestFiles::getPathToFile("v_invalid_no_bps_tag.tif"),
                       "TitleGoesHere", 0, 0);
  // Tif handling should fail here
  BOOST_REQUIRE(!test_varnishLayer->fillMetaFromTiff(8, 1));
  // Properties set correctly?
  BOOST_REQUIRE(test_varnishLayer->name == "TitleGoesHere");
  BOOST_REQUIRE(test_varnishLayer->filepath ==
                TestFiles::getPathToFile("v_invalid_no_bps_tag.tif"));
  BOOST_REQUIRE(test_varnishLayer->width == 0);
  BOOST_REQUIRE(test_varnishLayer->height == 0);
  BOOST_REQUIRE(test_varnishLayer->xoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->yoffset == 0);
  BOOST_REQUIRE(test_varnishLayer->xAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->yAspect == 0.0f);
  BOOST_REQUIRE(test_varnishLayer->bitmap == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()