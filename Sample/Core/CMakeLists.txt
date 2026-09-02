find_package(nlohmann_json CONFIG REQUIRED)

add_library(SampleCore)
add_library(Sample::Core ALIAS SampleCore)

set_target_properties(SampleCore PROPERTIES
        EXPORT_NAME Core
)

target_link_libraries(SampleCore
        PRIVATE
        nlohmann_json::nlohmann_json
        $<BUILD_INTERFACE:std>
        PUBLIC
        Sample::Error
)

configure_file(
        "src/sample/core/version/version_constant.cppm.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/sample/version/version_constant.cppm"
        @ONLY
)

target_sources(SampleCore
        PRIVATE
        include/sample/compiler_fix.h
        src/sample/core/console_parser/console_parser.cpp
        src/sample/core/log/log.cpp
        src/sample/core/file/json/json.cpp
        src/sample/core/log_manager/log_manager.cpp
        src/sample/core/event_loop/event_loop.cpp
        PUBLIC
        FILE_SET generated_modules TYPE CXX_MODULES BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/generated" FILES
        "${CMAKE_CURRENT_BINARY_DIR}/generated/sample/version/version_constant.cppm"
        FILE_SET source_modules TYPE CXX_MODULES BASE_DIRS src FILES
        src/sample/core/event_listener/event_listener.cppm
        src/sample/core/event_loop/event_loop.cppm
        src/sample/core/console_parser/console_parser.cppm
        src/sample/core/console_parser/console_parser_constant.cppm
        src/sample/core/log/log.cppm
        src/sample/core/log/log_constant.cppm
        src/sample/core/file/json/json.cppm
        src/sample/core/file/file_guard/file_guard.cppm
        src/sample/core/thread_safe_data/thread_safe_data.cppm
        src/sample/core/version/version.cppm
        src/sample/core/log_manager/log_manager.cppm
        src/sample/core/string_unordered_map/string_unordered_map.cppm
)

target_include_directories(SampleCore PRIVATE
        "${Stb_INCLUDE_DIR}"
)

target_include_directories(SampleCore PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS SampleCore
        EXPORT SampleCoreTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET generated_modules DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleCore/modules"
        FILE_SET source_modules DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleCore/modules"
)

install(EXPORT SampleCoreTargets
        FILE SampleCoreTargets.cmake
        NAMESPACE Sample::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleCore"
        CXX_MODULES_DIRECTORY modules
)

if (BUILD_SHARED_LIBS)
    set(SampleCore_IS_SHARED TRUE)
else ()
    set(SampleCore_IS_SHARED FALSE)
endif ()

configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SampleCoreConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/SampleCoreConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleCore"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SampleCoreConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SampleCore")

install(DIRECTORY "include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")
