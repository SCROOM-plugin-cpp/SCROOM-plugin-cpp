add_library(spconfig SHARED CustomColor.hh CustomColorOperations.cc CustomColorOperations.hh)
target_sources(spconfig PRIVATE
        main.cc
        CustomColorConfig.cc
        CustomColorConfig.hh
        CustomColor.hh
        CustomColorOperations.cc
        CustomColorOperations.hh)

target_link_libraries(
        spconfig
        PRIVATE project_options project_warnings
        PUBLIC PkgConfig::gtk PkgConfig::cairo
        Boost::filesystem
        scroom_lib)

install(TARGETS spconfig DESTINATION ${PLUGIN_INSTALL_LOCATION_RELATIVE})
install_plugin_dependencies(spconfig)
