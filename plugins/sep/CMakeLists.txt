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
        boost_filesystem
        scroom_lib
        TIFF::TIFF
        m
        util
        dl)

install(TARGETS spsep DESTINATION ${PLUGIN_INSTALL_LOCATION_RELATIVE})
install_plugin_dependencies(spsep)

message("spsep: " ${PLUGIN_INSTALL_LOCATION_RELATIVE})

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
          test/varnish-tests.cc)
  #target_include_directories(spsep_tests PRIVATE sli)
  target_link_libraries(
    spsep_tests
    PRIVATE boosttesthelper
            #Boost::chrono
            project_options
            project_warnings
            spsep
            tiledbitmap
            util)

  # We explicitly create a directory for the files beforehand
  # so we can just specify the directory in the configure_file command
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/testfiles)
  # Copy test files to build directory
  configure_file(test/testfiles/C.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/K.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/M.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/M_9.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sep_cmyk.sep ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sep_cmykv.sep ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sep_empty_line_2.sep ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sep_empty_line_3.sep ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sep_extra_lines.sep ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sep_missing_channel.sep ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sep_test.sep ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_pipette.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_scale.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_seponly.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_septiffmixed.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_tiffonly.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_tinycmyk.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_tinycmyk_xoffset.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_varnish.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_varnish_wrongpath.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/sli_xoffset.sli ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/tiff_cmyk.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/tinycmyk.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/v_corrupted.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/v_invalid1bps.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/v_invalid_no_bps_tag.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/v_invalid_no_width.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/v_invalid_zero_res.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/v_invalidrgb.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/v_valid.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/v_valid_centimeter.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/v_valid_no_spp_tag.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)
  configure_file(test/testfiles/Y.tif ${CMAKE_BINARY_DIR}/testfiles COPYONLY)

  add_test(NAME spsep_tests COMMAND spsep_tests)
endif()
