cmake_minimum_required(VERSION 3.15)
project(ScroomCPPPlugins LANGUAGES CXX)

option(ENABLE_BOOST_TEST "Enable Boost Test builds" ON)

# Collect all libraries and binaries in a single location
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

add_subdirectory(scroom)
add_subdirectory(plugins)

# Generate the ENVIRONMENT file
set(abs_top_builddir "${ScroomCPPPlugins_BINARY_DIR}")
configure_file(ENVIRONMENT.in ${ScroomCPPPlugins_BINARY_DIR}/ENVIRONMENT @ONLY)
