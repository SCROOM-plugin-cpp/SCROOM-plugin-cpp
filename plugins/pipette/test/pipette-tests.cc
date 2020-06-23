#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>

// Make all private members accessible for testing
#define private public

#include "../src/pipette.hh"

BOOST_AUTO_TEST_SUITE(Pipette_Tests)

BOOST_AUTO_TEST_CASE(enable_test) {
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
