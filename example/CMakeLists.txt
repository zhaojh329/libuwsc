set(LIBS ${LIBEV_LIBRARY} uwsc)

if(SSL_SUPPORT)
    list(APPEND LIBS ${SSL_LIBS})
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/buffer
    ${CMAKE_SOURCE_DIR}/src/log
    ${CMAKE_BINARY_DIR}/src
    ${LIBEV_INCLUDE_DIR})

add_executable(example example.c)
target_link_libraries(example PRIVATE ${LIBS})
