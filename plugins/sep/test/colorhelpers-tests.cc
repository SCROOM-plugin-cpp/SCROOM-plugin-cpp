//
// Created by developer on 18-06-21.
//

#include <boost/test/unit_test.hpp>
#define private public

#include "../colorconfig/CustomColorHelpers.hh"
#include "testglobals.hh"

BOOST_AUTO_TEST_SUITE(ColorHelpers_Tests)

BOOST_AUTO_TEST_CASE(colorHelpers_toUint8_lower) {
  int32_t value = -1;
  CustomColorHelpers helpers;
  BOOST_CHECK_EQUAL(helpers.toUint8(value), 0);
}

BOOST_AUTO_TEST_CASE(colorHelpers_toUint8_upper) {
  int32_t value = 420;
  CustomColorHelpers helpers;
  BOOST_CHECK_EQUAL(helpers.toUint8(value), 255);
}

BOOST_AUTO_TEST_CASE(colorHelpers_toUint8_inrange) {
  int32_t value = 42;
  CustomColorHelpers helpers;
  BOOST_CHECK_EQUAL(helpers.toUint8(value), 42);
}

BOOST_AUTO_TEST_CASE(colorHelpers_calculateCMYK) {
  CustomColor::Ptr color(new CustomColor("test", 1, 1, 1, 1));
  CustomColorHelpers helpers;
  int16_t c = 1;
  helpers.calculateCMYK(color, c, c, c, c, 1);
  BOOST_CHECK_EQUAL(c, 5);
}

BOOST_AUTO_TEST_CASE(colorHelpers_calculateCMYKall) {
  CustomColor::Ptr color(new CustomColor("test", 1, 1, 1, 1));
  CustomColorHelpers helpers;
  int16_t c = 1;
  int16_t m = 2;
  int16_t y = 3;
  int16_t k = 4;
  helpers.calculateCMYK(color, c, m, y, k, 1);

  bool correct = true;
  if (c != 2 || m != 3 || y != 4 || k != 5) {
    correct = false;
  }
  BOOST_CHECK(correct);
}

BOOST_AUTO_TEST_SUITE_END()