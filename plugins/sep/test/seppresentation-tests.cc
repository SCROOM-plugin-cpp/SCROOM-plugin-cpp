#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

// Make all private members accessible for testing
#define private public

#include "../seppresentation.hh"
#include "../sepsource.hh"

const auto testFileDir = boost::dll::program_location().parent_path().parent_path() / "testfiles";

BOOST_AUTO_TEST_SUITE(SepPresentation_Tests)

BOOST_AUTO_TEST_CASE(create) {
    SepPresentation::Ptr presentation = SepPresentation::create();
    BOOST_CHECK(presentation != nullptr);
}

BOOST_AUTO_TEST_CASE(load_1) {
    SepPresentation::Ptr presentation = SepPresentation::create();
    BOOST_CHECK_EQUAL(presentation->load((testFileDir / "sep_test.sep").string()), false);
}

BOOST_AUTO_TEST_CASE(load_2) {
    SepPresentation::Ptr presentation = SepPresentation::create();
    BOOST_CHECK_EQUAL(presentation->load((testFileDir / "sep_cmyk.sep").string()), true);
}

BOOST_AUTO_TEST_CASE(getTitle) {
    SepPresentation::Ptr presentation = SepPresentation::create();
    BOOST_CHECK_EQUAL(presentation->load((testFileDir / "sep_cmyk.sep").string()), true);
    BOOST_CHECK_EQUAL(presentation->getTitle(), (testFileDir / "sep_cmyk.sep").string());
}

BOOST_AUTO_TEST_CASE(getTransform) {
    SepPresentation::Ptr presentation = SepPresentation::create();
    BOOST_CHECK_EQUAL(presentation->load((testFileDir / "sep_cmyk.sep").string()), true);
    BOOST_CHECK(presentation->getTransform() != nullptr);
}

BOOST_AUTO_TEST_CASE(getRect) {
    SepPresentation::Ptr presentation = SepPresentation::create();
    BOOST_CHECK_EQUAL(presentation->load((testFileDir / "sep_cmyk.sep").string()), true);
    auto rect = presentation->getRect();
    BOOST_CHECK(rect.getTop() == 0);
    BOOST_CHECK(rect.getLeft() == 0);
    BOOST_CHECK(rect.getWidth() == 600);
    BOOST_CHECK(rect.getHeight() == 400);
}


BOOST_AUTO_TEST_SUITE_END()