cmake_minimum_required(VERSION 2.8)

project(libuwsc C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

add_subdirectory(src)
add_subdirectory(example)
