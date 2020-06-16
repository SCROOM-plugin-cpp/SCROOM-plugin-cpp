#include "../sepsource.hh"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Sep_Tests)

BOOST_AUTO_TEST_CASE(create_1) {
    auto source = SepSource::create();
    BOOST_CHECK(source != nullptr);
}