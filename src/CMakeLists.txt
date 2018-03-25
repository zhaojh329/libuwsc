
add_definitions(-O -Wall -Werror --std=gnu99 -D_GNU_SOURCE -Wno-misleading-indentation)

# The version number.
set(UWSC_VERSION_MAJOR 2)
set(UWSC_VERSION_MINOR 0)
set(UWSC_VERSION_PATCH 0)

# Check the third party Libraries
find_package(Libubox REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${LIBUBOX_INCLUDE_DIR})

set(EXTRA_LIBS ${LIBUBOX_LIBRARY} dl)
set(SOURCE_FILES uwsc.c log.c utils.c)

set(UWSC_SSL_SUPPORT_CONFIG 1)
option(UWSC_SSL_SUPPORT "SSL support" ON)

if(NOT UWSC_SSL_SUPPORT)
    set(UWSC_SSL_SUPPORT_CONFIG 0)
endif()

add_library(uwsc SHARED ${SOURCE_FILES})
set_target_properties(uwsc PROPERTIES VERSION ${UWSC_VERSION_MAJOR}.${UWSC_VERSION_MINOR}.${UWSC_VERSION_PATCH})
target_link_libraries(uwsc ${EXTRA_LIBS})

# configure a header file to pass some of the CMake settings to the source code
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

install(
    FILES
	    ${CMAKE_CURRENT_BINARY_DIR}/config.h
	    ${CMAKE_CURRENT_SOURCE_DIR}/uwsc.h
    DESTINATION
    	include/uwsc
)

install(
    TARGETS uwsc LIBRARY
    DESTINATION lib
)

message("")
message(STATUS "UWSC_VERSION: ${UWSC_VERSION_MAJOR}.${UWSC_VERSION_MINOR}.${UWSC_VERSION_PATCH}")
message(STATUS "UWSC_SSL_SUPPORT: ${UWSC_SSL_SUPPORT}")
message("")
