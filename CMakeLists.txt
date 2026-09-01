cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_STANDARD_LIBRARY_MODULES ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(SampleProject
        VERSION 1.0.0
        LANGUAGES CXX)

include(cmake/SetupStdModule.cmake)

add_subdirectory(Sample)
add_subdirectory(AtService)
