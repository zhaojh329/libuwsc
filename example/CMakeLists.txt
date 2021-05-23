include_directories(${libuwsc_SOURCE_DIR}/src ${libuwsc_SOURCE_DIR}/src/buffer ${libuwsc_SOURCE_DIR}/src/log ${libuwsc_BINARY_DIR}/src ${LIBEV_INCLUDE_DIR})

add_executable(example example.c)
if(BUILD_SHARED_LIBS)
    target_link_libraries(example uwsc ${LIBEV_LIBRARY})
else()
    target_link_libraries(example uwsc_s ${LIBEV_LIBRARY})
endif()
