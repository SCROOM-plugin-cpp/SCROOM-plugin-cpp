#define BOOST_TEST_MODULE Sep tests
#define BOOST_TEST_DYN_LINK
#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>
#include "constants.hh"

// We use this struct as a global fixture to initialize the global
// variable testFileDir based on a command line argument containing the path
// to the directory containing the test files
namespace utf = boost::unit_test;
struct F {
    F() {
        BOOST_TEST_MESSAGE("setup fixture");
        if (utf::framework::master_test_suite().argc < 2){
            // Use a default location if no arguments were passed
            testFileDir = (boost::dll::program_location().parent_path().parent_path() / "testfiles").string();
        } else {
            testFileDir = utf::framework::master_test_suite().argv[1];
        }
        BOOST_TEST_MESSAGE("Using the test files located in " + testFileDir.string());
    }
    ~F() {
        BOOST_TEST_MESSAGE("teardown fixture");
    }
};
// Register the fixture with the test module
BOOST_GLOBAL_FIXTURE(F);