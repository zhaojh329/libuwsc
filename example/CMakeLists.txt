set(LIBS ${LIBEV_LIBRARY} uwsc)

if(SSL_SUPPORT)
    list(APPEND LIBS ${SSL_LIBS})
endif()

include_directories(
    ${libuwsc_SOURCE_DIR}/src
    ${libuwsc_SOURCE_DIR}/src/buffer
    ${libuwsc_SOURCE_DIR}/src/log
    ${libuwsc_BINARY_DIR}/src
    ${LIBEV_INCLUDE_DIR})

add_executable(example example.c)
target_link_libraries(example PRIVATE ${LIBS})
