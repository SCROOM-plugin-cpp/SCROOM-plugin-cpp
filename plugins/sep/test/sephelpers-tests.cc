#include <boost/test/unit_test.hpp>

// Make all private members accessible for testing
#define private public

#include "../sep-helpers.hh"

BOOST_AUTO_TEST_SUITE(SepHelpers_Tests)

BOOST_AUTO_TEST_CASE(sephelpers_sum_empty_pipette_colours) {
  auto res = sumPipetteColors({}, {{"C", 1.0}});

  BOOST_CHECK(res.size() == 1);
  BOOST_CHECK(res[0].first == "C");
  BOOST_CHECK(std::abs(res[0].second - 1.0) < 1e-4);
}

BOOST_AUTO_TEST_CASE(sephelpers_sum_pipette_colours) {
  auto res = sumPipetteColors({{"C", 1.0}}, {{"C", 1.0}});
  BOOST_CHECK(res.size() == 1);
  BOOST_CHECK(res[0].first == "C");
  BOOST_CHECK(std::abs(res[0].second - 2.0) < 1e-4);
}

BOOST_AUTO_TEST_CASE(sephelpers_divide_pipette_colours) {
  auto res = dividePipetteColors({{"C", 4.0}}, 4);
  BOOST_CHECK(res.size() == 1);
  BOOST_CHECK(res[0].first == "C");
  BOOST_CHECK(std::abs(res[0].second - 1.0) < 1e-4);
}

BOOST_AUTO_TEST_SUITE_END()