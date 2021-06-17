#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

// Make all private members accessible for testing
#define private public

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
  ProgressInterface::Ptr getProgressInterface(){};

  static ViewInterface::Ptr create() {
    return ViewInterface::Ptr(new DummyViewInterface());
  }

  void addSideWidget(std::string title, GtkWidget *w) {
    // Add the widget to a fresh GTK box instead.
    // This won't verify UI integrity, but it should
    // at least allow the code to run once in headless CI
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), w, true, true, 0);
  }
  void removeSideWidget(GtkWidget *w){};
  void addToToolbar(GtkToolItem *ti){};
  void removeFromToolbar(GtkToolItem *ti){};
  void registerSelectionListener(SelectionListener::Ptr){};
  void registerPostRenderer(PostRenderer::Ptr){};
  void setStatusMessage(const std::string &){};
  boost::shared_ptr<PresentationInterface> getCurrentPresentation() {
    return nullptr;
  };
  void addToolButton(GtkToggleButton *, ToolStateListener::Ptr){};
};

void dummyFunction(){};

///////////////////////////////////////////////////////////////////////////////
// Tests for varnish ui construction

BOOST_AUTO_TEST_SUITE(varnish_ui_tests)
BOOST_AUTO_TEST_CASE(varnish_load_ui) {
  ViewInterface::Ptr dvi = DummyViewInterface::create();
  SliLayer::Ptr test_varnishLayer = SliLayer::create(
      TestFiles::getPathToFile("v_valid.tif"), "SomeCoolTitle", 0, 0);
  test_varnishLayer->fillMetaFromTiff(8, 1);
  test_varnishLayer->fillBitmapFromTiff();
  Varnish::Ptr test_varnish = Varnish::create(test_varnishLayer);
  test_varnish->triggerRedraw = boost::bind(dummyFunction);
  test_varnish->setView(dvi);
  test_varnish->invertSurface();

  // Inverting twice should return the same thing.
  // Inverting once shouldn't, because the sample image isn't gray
  int surface_data_before =
      cairo_image_surface_get_data(test_varnish->surface)[0];
  test_varnish->invertSurface();
  int surface_data_after =
      cairo_image_surface_get_data(test_varnish->surface)[0];
  BOOST_REQUIRE(surface_data_before == surface_data_after ^ 255);
  BOOST_REQUIRE(surface_data_before != surface_data_after);

  test_varnish->invertSurface();
  int surface_data_twice =
      cairo_image_surface_get_data(test_varnish->surface)[0];
  BOOST_REQUIRE(surface_data_after == surface_data_twice ^ 255);
  BOOST_REQUIRE(surface_data_after != surface_data_twice);
  BOOST_REQUIRE(surface_data_before == surface_data_twice);

  BOOST_REQUIRE(!test_varnish->inverted);
  // trigger a button callback
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(test_varnish->check_show_background), true);
  // drawoverlay
  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 100, 100);
  cairo_t *cr = cairo_create(surface);
  Scroom::Utils::Rectangle<double> rect(5.0, 5.0, 100.0, 100.0);
  // call the draw function at several zoom levels
  test_varnish->drawOverlay(nullptr, cr, rect, -1);
  test_varnish->drawOverlay(nullptr, cr, rect, 1);
  test_varnish->drawOverlay(nullptr, cr, rect, 2);
  // Disable transparency and redraw
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(test_varnish->check_show_background), false);
  test_varnish->drawOverlay(nullptr, cr, rect, -1);
  test_varnish->drawOverlay(nullptr, cr, rect, 1);
  test_varnish->drawOverlay(nullptr, cr, rect, 2);
  // Enable varnish and redraw
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test_varnish->radio_enabled),
                               true);
  BOOST_REQUIRE(!test_varnish->inverted);
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(test_varnish->check_show_background), true);
  test_varnish->drawOverlay(nullptr, cr, rect, -1);
  test_varnish->drawOverlay(nullptr, cr, rect, 1);
  test_varnish->drawOverlay(nullptr, cr, rect, 2);
  // Disable transparency and redraw
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(test_varnish->check_show_background), false);
  test_varnish->drawOverlay(nullptr, cr, rect, -1);
  test_varnish->drawOverlay(nullptr, cr, rect, 1);
  test_varnish->drawOverlay(nullptr, cr, rect, 2);
  // Invert varnish and redraw
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test_varnish->radio_inverted),
                               true);
  BOOST_REQUIRE(test_varnish->inverted);
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(test_varnish->check_show_background), true);
  test_varnish->drawOverlay(nullptr, cr, rect, -1);
  test_varnish->drawOverlay(nullptr, cr, rect, 1);
  test_varnish->drawOverlay(nullptr, cr, rect, 2);
  // Disable transparency and redraw
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(test_varnish->check_show_background), false);
  test_varnish->drawOverlay(nullptr, cr, rect, -1);
  test_varnish->drawOverlay(nullptr, cr, rect, 1);
  test_varnish->drawOverlay(nullptr, cr, rect, 2);
  // Move from inverted to normal and redraw
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(test_varnish->radio_enabled),
                               true);
  BOOST_REQUIRE(!test_varnish->inverted);
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(test_varnish->check_show_background), true);
  test_varnish->drawOverlay(nullptr, cr, rect, -1);
  test_varnish->drawOverlay(nullptr, cr, rect, 1);
  test_varnish->drawOverlay(nullptr, cr, rect, 2);
}
BOOST_AUTO_TEST_SUITE_END()

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