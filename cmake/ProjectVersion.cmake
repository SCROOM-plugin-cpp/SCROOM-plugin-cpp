if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  git_describe(GIT_CPP_VERSION --tags)

  file(
    GENERATE
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/.cpp-plugin-version
    CONTENT "${GIT_CPP_VERSION}")
else()
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/.cpp-plugin-version GIT_CPP_VERSION)
endif()

file(
  GENERATE
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpp-version.h
  CONTENT "#define PACKAGE_VERSION \"${GIT_CPP_VERSION}\"")
