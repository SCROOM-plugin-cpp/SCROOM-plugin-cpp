#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

#include <iostream>

// Make all private members accessible for testing
#define private public
#include "../sli/slilayer.hh"
#include "../sli/sli-helpers.hh"
#include <scroom/scroominterface.hh>


///////////////////////////////////////////////////////////////////////////////
// Helper functions


///////////////////////////////////////////////////////////////////////////////
// Tests

BOOST_AUTO_TEST_SUITE(Sli_Tests)

// clearSurface(Scroom::Utils::Rectangle<int> rect);
BOOST_AUTO_TEST_CASE(slihelpers_clear_surface)
{
  unsigned int width = 10;
  unsigned int height = 10;
  auto surfaceWrapper = SurfaceWrapper::create(width, height, CAIRO_FORMAT_ARGB32); // 40x10
  cairo_t* cr = cairo_create(surfaceWrapper->surface);
  cairo_set_source_rgba(cr, 1,1,1,1);
  cairo_paint(cr); // paint everything whites
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
  uint8_t* data = cairo_image_surface_get_data(surfaceWrapper->surface);
  Scroom::Utils::Rectangle<int> rect = {0,0,5,10}; // in bytes
  bool allWhite= true;
  bool allClear = true;
  bool rectClear = true;

  // Check all pixels are white
  for (unsigned int i = 0; i < height * stride; i+=(stride/width))
  {
    if (data[i] != 255)
    {
      allWhite = false;
      break;
    }
  }
  BOOST_CHECK(allWhite == true);

  surfaceWrapper->clearSurface();

  // Checks all pixels have been cleared (ie = 0)
  for (unsigned int i = 0; i < height * stride; i++)
  {
    if (data[i] != 0)
    {
      allClear = false;
      break;
    }
  }
  BOOST_CHECK(allClear == true);

  cairo_paint(cr); // paint everything white again

  // Check all pixels are white
  for (unsigned int i = 0; i < height * stride; i++)
  {
    if (data[i] != 255)
    {
      allWhite = false;
      break;
    }
  }
  BOOST_CHECK(allWhite == true);

  surfaceWrapper->clearSurface(rect);

  // Checks that the pixels inside the rectangles have been cleared
  // and that those outside are still white
  for (unsigned int i = 0; i < height * stride; i++)
  {
    if ((i % stride) < 5*(stride/width)) // clear part
    {
      if (data[i] != 0)
      {
        rectClear = false;
        break;
      }
    }
    else
    {
      if (data[i] != 255)
      {
        rectClear = false;
        break;
      }
    }
  }
  BOOST_CHECK(rectClear == true);
  cairo_destroy(cr);
}

BOOST_AUTO_TEST_CASE(slihelpers_get_area)
{
  Scroom::Utils::Rectangle<int> rect1 {0, 0, 6969, 420};
  Scroom::Utils::Rectangle<int> rect2 {0, 0, 0, 0};
  Scroom::Utils::Rectangle<int> rect3 {0, 1, 2, 2};
  Scroom::Utils::Rectangle<int> rect4 {1, 0, 1, 1};

  BOOST_CHECK(getArea(rect1) == 2926980);
  BOOST_CHECK(getArea(rect2) == 0);
  BOOST_CHECK(getArea(rect3) == 4);
  BOOST_CHECK(getArea(rect4) == 1);
}

BOOST_AUTO_TEST_CASE(slihelpers_to_bytes_rectangle)
{
  Scroom::Utils::Rectangle<int> rect1 {10, 10, 100, 100};
  auto bRect = toBytesRectangle(rect1);
  
  BOOST_CHECK(bRect.getWidth() == 400);
  BOOST_CHECK(bRect.getHeight() == 100);
  BOOST_CHECK(bRect.getLeft() == 40);
  BOOST_CHECK(bRect.getTop() == 10);
}

BOOST_AUTO_TEST_CASE(slihelpers_point_to_offset)
{
  Scroom::Utils::Point<int> p1 {2, 2};
  int stride = 5;
  Scroom::Utils::Rectangle<int> rect {2, 2, 5, 5};
  Scroom::Utils::Point<int> p2 {3, 3};
  
  BOOST_CHECK(pointToOffset(p1, stride) == 12);
  BOOST_CHECK(pointToOffset(rect, p2) == 6);
}

BOOST_AUTO_TEST_CASE(slihelpers_spanned_rectangle)
{
  unsigned int n_layers = 2;
  boost::dynamic_bitset<> bitmap {n_layers};
  std::vector<SliLayer::Ptr> layers;

  layers.push_back(SliLayer::create("", "Layer", 0, 0));
  layers[0]->xoffset = 0;
  layers[0]->yoffset = 0;
  layers[0]->width = 100;
  layers[0]->height = 100;
  bitmap.set(0);

  auto spanRect1 = spannedRectangle(bitmap, layers, false);
  BOOST_CHECK(getArea(spanRect1) == 100*100);

  layers.push_back(SliLayer::create("", "Layer", 0, 0));
  layers[1]->xoffset = 10;
  layers[1]->yoffset = 10;
  layers[1]->width = 100;
  layers[1]->height = 100;
  bitmap.set(1);

  auto spanRect2 = spannedRectangle(bitmap, layers, false);
  BOOST_CHECK(getArea(spanRect2) == 110*110);
}

BOOST_AUTO_TEST_SUITE_END()