get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(TOOLSET_ROOT "${COMPILER_DIR}/.." ABSOLUTE)

file(GLOB_RECURSE FOUND_STD_LIST
        FOLLOW_SYMLINKS
        LIST_DIRECTORIES false
        "${TOOLSET_ROOT}/*/std.cc"
)

message(STATUS "Sample: Found std module source at toolset: ${TOOLSET_ROOT}")

if (FOUND_STD_LIST)
    list(GET FOUND_STD_LIST 0 STD_SOURCE)
    get_filename_component(BITS_DIR "${STD_SOURCE}" DIRECTORY)
    get_filename_component(STD_BASE_DIR "${BITS_DIR}/.." ABSOLUTE)
    message(STATUS "Sample: Found std module source at: ${STD_SOURCE}")
    add_library(std STATIC)
    target_sources(std
            PUBLIC
            FILE_SET std_set TYPE CXX_MODULES
            BASE_DIRS "${STD_BASE_DIR}"
            FILES "${STD_SOURCE}"
    )
else ()
    message(WARNING "Sample: std.cc not found. Built-in 'import std' might fail.")
endif ()