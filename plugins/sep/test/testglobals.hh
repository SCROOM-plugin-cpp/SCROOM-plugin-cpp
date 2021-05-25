#pragma once

#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>

namespace utf = boost::unit_test;
/**
 * This struct is used as a global struct for the SEP test module and exposes methods to get paths to test files.
 * It reads the first argument passed to the test module as the path to the test file directory.
 * If no arguments were passed it uses the default directory <binary_location>/../../testfiles.
 * See: https://www.boost.org/doc/libs/1_76_0/libs/test/doc/html/boost_test/runtime_config/custom_command_line_arguments.html
 */
struct TestFiles {

private:
    inline static boost::filesystem::path dir;

public:
    TestFiles() {
        BOOST_TEST_MESSAGE("Setting up the TestFiles fixture");
        if (utf::framework::master_test_suite().argc < 2){
            // Use a default location if no arguments were passed
            dir = (boost::dll::program_location().parent_path().parent_path() / "testfiles");
        } else {
            dir = utf::framework::master_test_suite().argv[1];
        }
        BOOST_TEST_MESSAGE("Using the test files located in " + dir.string());
    }

    /**
     * Returns the path to the test files directory.
     * @return The path to the test files directory.
     */
    static std::string getPath() {
        return dir.string();
    }

    /**
     * Returns the path to the file with the given filename.
     * @param filename Name of the file in the test files directory.
     * @return The path to the file with the given filename.
     */
    static std::string getPathToFile(std::string filename) {
        return (dir / filename).string();
    }
};
