add_library(spsep SHARED)
target_sources(spsep
        PRIVATE
        main.cc
        sep.cc
        sep.hh
        sep-helpers.cc
        sep-helpers.hh
        seppresentation.cc
        seppresentation.hh
        sepsource.cc
        sepsource.hh
        sli/sli-helpers.cc
        sli/sli-helpers.hh
        sli/slicontrolpanel.cc
        sli/slicontrolpanel.hh
        sli/slilayer.cc
        sli/slilayer.hh
        sli/slipresentation.cc
        sli/slipresentation.hh
        sli/slipresentationinterface.hh
        sli/slisource.cc
        sli/slisource.hh
        varnish/varnish.cc
        varnish/varnish.hh)
target_link_libraries(
  spsep
        PRIVATE project_options project_warnings
        PUBLIC PkgConfig::gtk threadpool PkgConfig::cairo
        Boost::filesystem
		tiledbitmap
        scroom_lib
        TIFF::TIFF
        m
        util
        dl)

install(TARGETS spsep DESTINATION ${PLUGIN_INSTALL_LOCATION_RELATIVE})
install_plugin_dependencies(spsep)

if(ENABLE_BOOST_TEST)
  add_executable(spsep_tests)
  target_sources(spsep_tests
          PRIVATE
          test/main.cc
          test/sep-tests.cc
          test/sephelpers-tests.cc
          test/seppresentation-tests.cc
          test/sepsource-tests.cc
          test/slihelpers-tests.cc
          test/slipresentation-tests.cc
          test/slisource-tests.cc
          test/varnish-tests.cc
          test/constants.cc
          test/constants.hh)
  target_include_directories(spsep_tests PRIVATE sli)
  target_link_libraries(
    spsep_tests
    PRIVATE boosttesthelper
            #Boost::chrono
            project_options
            project_warnings
            spsep
            util)

  add_test(NAME spsep_tests COMMAND spsep_tests -- ${CMAKE_CURRENT_SOURCE_DIR}/test/testfiles)
endif()
