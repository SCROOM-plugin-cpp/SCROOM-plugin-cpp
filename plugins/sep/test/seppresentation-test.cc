#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>

// Make all private members accessible for testing
#define private public

#include "../seppresentation.hh"

BOOST_AUTO_TEST_SUITE(SepPresentation_Tests);

const auto testFileDir = boost::dll::program_location().parent_path().parent_path() / "testfiles";

BOOST_AUTO_TEST_CASE(number1) {
    SepPresentation::Ptr presentation = SepPresentation::create();
}