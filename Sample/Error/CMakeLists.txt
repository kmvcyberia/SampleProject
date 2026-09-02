add_library(SampleError)
add_library(Sample::Error ALIAS SampleError)

set_target_properties(SampleError PROPERTIES
        EXPORT_NAME Error
)

target_link_libraries(SampleError
        PRIVATE
        $<BUILD_INTERFACE:std>
)

target_sources(SampleError
        PUBLIC
        FILE_SET source_modules TYPE CXX_MODULES BASE_DIRS src FILES
        src/sample/error/error.cppm
        src/sample/error/error_system.cppm
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS SampleError
        EXPORT SampleErrorTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET source_modules DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleError/modules"
)

install(EXPORT SampleErrorTargets
        FILE SampleErrorTargets.cmake
        NAMESPACE Sample::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleError"
        CXX_MODULES_DIRECTORY modules
)

configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SampleErrorConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/SampleErrorConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleError"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SampleErrorConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleError")