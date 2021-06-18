//
// Created by developer on 17-06-21.
//

#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>
#define private public

#include "../colorconfig/CustomColorConfig.hh"
#include "testglobals.hh"

BOOST_AUTO_TEST_SUITE(ColorConfig_Tests)

BOOST_AUTO_TEST_CASE(colorConfig_create) {
  ColorConfig();
  ColorConfig colorConfig;
  BOOST_CHECK(colorConfig.getDefinedColors().size() == 0);
}

BOOST_AUTO_TEST_CASE(colorConfig_load_without_file) {
  ColorConfig colorConfig;

  colorConfig.loadFile();
  BOOST_CHECK(colorConfig.getDefinedColors().size() == 4);
}

BOOST_AUTO_TEST_CASE(colorConfig_load_with_file) {
  ColorConfig colorConfig;

  colorConfig.loadFile(TestFiles::getPathToFile("colours.json"));
  BOOST_CHECK(colorConfig.getDefinedColors().size() == 5);
}

BOOST_AUTO_TEST_CASE(colorConfig_non_existent) {
  ColorConfig colorConfig;

  colorConfig.addNonExistentDefaultColors();
  BOOST_CHECK(colorConfig.getDefinedColors().size() == 4);
}

BOOST_AUTO_TEST_CASE(colorConfig_get_default_name) {
  ColorConfig colorConfig;

  colorConfig.addNonExistentDefaultColors();
  BOOST_CHECK(colorConfig.getColorByNameOrAlias("c") != nullptr);
}

BOOST_AUTO_TEST_CASE(colorConfig_get_name) {
  ColorConfig colorConfig;

  colorConfig.loadFile(TestFiles::getPathToFile("colours.json"));
  BOOST_CHECK(colorConfig.getColorByNameOrAlias("b") != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()