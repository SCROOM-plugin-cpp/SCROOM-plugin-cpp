#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/tools/output_test_stream.hpp>

#include <boost/dll.hpp>


// Make all private members accessible for testing
#define private public
#include "../sli/slipresentation.hh"
#include <scroom/scroominterface.hh>

const std::string testFileDir = boost::dll::program_location().parent_path().parent_path().string() + "/testfiles/";

void checkImageLoaded(SliPresentation::Ptr presentation)
{
  BOOST_REQUIRE(presentation->source->layers.size());
}

BOOST_AUTO_TEST_SUITE(Sli_Tests)

BOOST_AUTO_TEST_CASE(dummy)
{
  bool x = true;
  BOOST_REQUIRE(x);
}

BOOST_AUTO_TEST_CASE(slipresentation_load_sli_seponly)
{
  boost::test_tools::output_test_stream output;
  BOOST_TEST_MESSAGE(testFileDir);
  SliPresentation::Ptr presentation = SliPresentation::create(nullptr);
  presentation->load(testFileDir + "sli_tiffonly.sli");
  checkImageLoaded(presentation);
}

BOOST_AUTO_TEST_SUITE_END()
