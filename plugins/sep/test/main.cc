#define BOOST_TEST_MODULE Sep tests
#define BOOST_TEST_DYN_LINK
#include "testglobals.hh"
#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>

// Register the fixture with the test module
BOOST_TEST_GLOBAL_FIXTURE(TestFiles);