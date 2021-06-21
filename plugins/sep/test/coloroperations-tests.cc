//
// Created by developer on 18-06-21.
//

#include <boost/test/unit_test.hpp>
#include <scroom/bitmap-helpers.hh>
#define private public

#include "../colorconfig/CustomColorOperations.hh"
#include "testglobals.hh"

BOOST_AUTO_TEST_SUITE(ColorOperations_Tests)

BOOST_AUTO_TEST_CASE(colorOperations_create) {
  PipetteCommonOperationsCustomColor::Ptr colorOperations =
      OperationsCustomColors::create(4);
  BOOST_CHECK(colorOperations != nullptr);
}

BOOST_AUTO_TEST_CASE(colorOperations_setspp) {
  OperationsCustomColors operations(8);
  BOOST_CHECK(operations.getBpp() == 64);
}
/*
BOOST_AUTO_TEST_CASE(colorOperations_cache) {
  OperationsCustomColors operations(1);
  Scroom::MemoryBlobs::RawPageData::ConstPtr data;
  data = {};
  boost::shared_ptr<ConstTile> ptr(new ConstTile(1, 1, 8, data));
  Scroom::Utils::Stuff result = operations.cache(ptr);
  BOOST_CHECK(result != nullptr);
}

BOOST_AUTO_TEST_CASE(colorOperations_reduce) {
  OperationsCustomColors operations(1);
  Scroom::MemoryBlobs::RawPageData::Ptr data;
  data = {};
  boost::shared_ptr<Tile> tile(new Tile(1, 1, 8, data));
  Scroom::MemoryBlobs::RawPageData::ConstPtr constData;
  constData = {};
  boost::shared_ptr<ConstTile> ptr(new ConstTile(1, 1, 8, constData));
  operations.reduce(tile, ptr, 0, 0);
  // TODO figure out how to test reduce
}
*/
BOOST_AUTO_TEST_SUITE_END()