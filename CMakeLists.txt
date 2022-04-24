cmake_minimum_required(VERSION 3.15)
project(ScroomCPPPlugins LANGUAGES CXX)

set(SCROOM_SRC_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/scroom"
    CACHE PATH "Location of the Scroom sources")

list(APPEND CMAKE_MODULE_PATH "${SCROOM_SRC_DIR}/cmake-modules")
include(UseBackportedModules)
include(GetGitRevisionDescription)

include(cmake/ProjectVersion.cmake)

option(ENABLE_BOOST_TEST "Enable Boost Test builds" ON)

# Collect all libraries and binaries in a single location
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(SCROOM_BUILD_OUTPUT_DIR
    ${PROJECT_BINARY_DIR}
    CACHE FILEPATH "Directory where the build output is to be stored")

if(ENABLE_BOOST_TEST OR ENABLE_GOOGLE_TEST)
  enable_testing()
endif()

add_subdirectory(${SCROOM_SRC_DIR} scroom)
scroom_output_dirs_setup()

target_include_directories(project_options INTERFACE inc ${PROJECT_BINARY_DIR})
add_subdirectory(plugins)

set(CPACK_PACKAGE_VERSION "${GIT_CPP_VERSION}")
set(CPACK_SOURCE_GENERATOR "TGZ")
include(CPack)
