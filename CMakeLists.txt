cmake_minimum_required(VERSION 2.8)

project(libuwsc C)

option(BUILD_EXAMPLE "Build example" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

add_subdirectory(src)
if(BUILD_EXAMPLE)
    add_subdirectory(example)
endif()
