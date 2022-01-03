#include <gtk/gtk.h>

#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>
#include <list>

#include "../sep.hh"

/** Test cases for sep.hh */

BOOST_AUTO_TEST_SUITE(Sep_Tests)

BOOST_AUTO_TEST_CASE(sep_create) {
  Sep::Ptr sep = Sep::create();
  BOOST_CHECK(sep != nullptr);
}

BOOST_AUTO_TEST_CASE(sep_getPluginName) {
  Sep::Ptr sep = Sep::create();
  BOOST_CHECK(sep->getPluginName() == "SEP and SLI");
}

BOOST_AUTO_TEST_CASE(sep_getVersion) {
  Sep::Ptr sep = Sep::create();
  BOOST_CHECK(!sep->getPluginVersion().empty());
}

BOOST_AUTO_TEST_CASE(sep_filters_name) {
  Sep::Ptr sep = Sep::create();
  auto filters = sep->getFilters();
  BOOST_CHECK(filters.front() != nullptr);

  // should be the same since we only put one element
  BOOST_CHECK(filters.front() == filters.back());

  // check that the correct name is given
  BOOST_CHECK_EQUAL(
      strcmp(gtk_file_filter_get_name(filters.front()), "SEP/SLI files"), 0);
}

BOOST_AUTO_TEST_SUITE_END()