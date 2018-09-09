include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src)

add_executable(example example.c)
target_link_libraries(example uwsc ${LIBEV_LIBRARY})
