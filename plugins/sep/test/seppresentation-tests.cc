#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>

#include "../seppresentation.hh"
#include "testglobals.hh"

/** Test cases for seppresentation.hh */

BOOST_AUTO_TEST_SUITE(SepPresentation_Tests)

BOOST_AUTO_TEST_CASE(seppresentation_create) {
  SepPresentation::Ptr presentation = SepPresentation::create();
  BOOST_CHECK(presentation != nullptr);
}

BOOST_AUTO_TEST_CASE(seppresentation_load_false) {
  SepPresentation::Ptr presentation = SepPresentation::create();
  BOOST_CHECK_EQUAL(
      presentation->load(TestFiles::getPathToFile("sep_test.sep")), false);
  BOOST_CHECK_EQUAL(presentation->width, 0);
  BOOST_CHECK_EQUAL(presentation->height, 0);
}

BOOST_AUTO_TEST_CASE(seppresentation_load_false_2) {
  SepPresentation::Ptr presentation = SepPresentation::create();
  BOOST_CHECK_EQUAL(presentation->load(""), false);
  BOOST_CHECK_EQUAL(presentation->width, 0);
  BOOST_CHECK_EQUAL(presentation->height, 0);
}

BOOST_AUTO_TEST_CASE(seppresentation_load_true) {
  SepPresentation::Ptr presentation = SepPresentation::create();
  BOOST_CHECK_EQUAL(
      presentation->load(TestFiles::getPathToFile("sep_cmyk.sep")), true);
  BOOST_CHECK_EQUAL(presentation->width, 600);
  BOOST_CHECK_EQUAL(presentation->height, 400);
  for (auto c : presentation->sep_source->channels) {
    BOOST_CHECK(presentation->sep_source->channel_files[c] != nullptr);
  }
}

BOOST_AUTO_TEST_CASE(seppresentation_getTitle) {
  SepPresentation::Ptr presentation = SepPresentation::create();
  BOOST_CHECK_EQUAL(
      presentation->load(TestFiles::getPathToFile("sep_cmyk.sep")), true);
  BOOST_CHECK_EQUAL(presentation->getTitle(),
                    TestFiles::getPathToFile("sep_cmyk.sep"));
}

BOOST_AUTO_TEST_CASE(seppresentation_getTransform) {
  SepPresentation::Ptr presentation = SepPresentation::create();
  BOOST_CHECK_EQUAL(
      presentation->load(TestFiles::getPathToFile("sep_cmyk.sep")), true);
  BOOST_CHECK(presentation->getTransform() != nullptr);
}

BOOST_AUTO_TEST_CASE(seppresentation_getRect) {
  SepPresentation::Ptr presentation = SepPresentation::create();
  BOOST_CHECK_EQUAL(
      presentation->load(TestFiles::getPathToFile("sep_cmyk.sep")), true);
  auto rect = presentation->getRect();
  BOOST_CHECK(rect.getTop() == 0);
  BOOST_CHECK(rect.getLeft() == 0);
  BOOST_CHECK(rect.getWidth() == 600);
  BOOST_CHECK(rect.getHeight() == 400);
}

BOOST_AUTO_TEST_CASE(seppresentation_getViews) {
  SepPresentation::Ptr presentation = SepPresentation::create();
  auto views = presentation->getViews();
  BOOST_CHECK(views.empty());
}

BOOST_AUTO_TEST_CASE(seppresentation_pipette) {
  SepPresentation::Ptr presentation = SepPresentation::create();
  BOOST_CHECK_EQUAL(
      presentation->load(TestFiles::getPathToFile("sep_cmyk.sep")), true);
  auto rect = presentation->getRect();
  BOOST_CHECK(rect.getTop() == 0);
  BOOST_CHECK(rect.getLeft() == 0);
  BOOST_CHECK(rect.getWidth() == 600);
  BOOST_CHECK(rect.getHeight() == 400);

  for (auto c : presentation->sep_source->channels) {
    BOOST_CHECK(presentation->sep_source->channel_files[c] != nullptr);
  }

  auto averages = presentation->getPixelAverages(rect);

  for (auto &avg : averages) {
    BOOST_CHECK(!avg.first.empty());
    BOOST_CHECK(!std::isnan(avg.second));
  }
}

BOOST_AUTO_TEST_SUITE_END()