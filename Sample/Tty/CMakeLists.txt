add_library(SampleTty)
add_library(Sample::Tty ALIAS SampleTty)

set_target_properties(SampleTty PROPERTIES
        EXPORT_NAME Tty
)

target_link_libraries(SampleTty
        PRIVATE
        $<BUILD_INTERFACE:std>
        PUBLIC
        Sample::Core
        Sample::Error
)

target_sources(SampleTty
        PRIVATE
        src/sample/tty/dispatcher/dispatcher.cpp
        src/sample/tty/watcher/watcher.cpp
        src/sample/tty/handler/handler.cpp
        PUBLIC
        FILE_SET source_modules TYPE CXX_MODULES BASE_DIRS src FILES
        src/sample/tty/dispatcher/dispatcher.cppm
        src/sample/tty/watcher/watcher.cppm
        src/sample/tty/watcher/watcher_config.cppm
        src/sample/tty/handler/handler.cppm
        src/sample/tty/handler/handler_config.cppm
        src/sample/tty/type.cppm
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS SampleTty
        EXPORT SampleTtyTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET source_modules DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleTty/modules"
)

install(EXPORT SampleTtyTargets
        FILE SampleTtyTargets.cmake
        NAMESPACE Sample::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleTty"
        CXX_MODULES_DIRECTORY modules
)

configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SampleTtyConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/SampleTtyConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleTty"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SampleTtyConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleTty")