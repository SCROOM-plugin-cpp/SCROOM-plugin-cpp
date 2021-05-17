#include "constants.hh"

#include <boost/dll.hpp>

extern const boost::filesystem::path testFileDir = boost::dll::program_location().parent_path().parent_path() / "testfiles";

