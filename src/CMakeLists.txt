
# The version number.
set(UWSC_VERSION_MAJOR 3)
set(UWSC_VERSION_MINOR 3)
set(UWSC_VERSION_PATCH 5)

aux_source_directory(. SOURCES)
aux_source_directory(log SOURCES)
aux_source_directory(buffer SOURCES)

if(BUILD_STATIC)
    if(SSL_SUPPORT)
        list(APPEND SOURCES ${SSL_SRC})
    endif()

    add_library(uwsc STATIC ${SOURCES})

    if(SSL_SUPPORT)
        target_compile_definitions(uwsc PRIVATE ${SSL_DEFINE})
        target_include_directories(uwsc PRIVATE ${SSL_INC})
    endif()
else()
    add_library(uwsc SHARED ${SOURCES})

    if(SSL_SUPPORT)
        target_link_libraries(uwsc PRIVATE ${SSL_TARGET})
    endif()

    target_link_libraries(uwsc PRIVATE ${LIBEV_LIBRARY})
    set_target_properties(uwsc PROPERTIES VERSION ${UWSC_VERSION_MAJOR}.${UWSC_VERSION_MINOR}.${UWSC_VERSION_PATCH})
endif()

target_include_directories(uwsc PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${LIBEV_INCLUDE_DIR} buffer log)

# configure a header file to pass some of the CMake settings to the source code
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(lua)

install(
    FILES
        log/log.h
        uwsc.h
        utils.h
        buffer/buffer.h
        ${CMAKE_CURRENT_BINARY_DIR}/config.h
    DESTINATION
        include/uwsc
)

message(STATUS "UWSC_VERSION: ${UWSC_VERSION_MAJOR}.${UWSC_VERSION_MINOR}.${UWSC_VERSION_PATCH}")
