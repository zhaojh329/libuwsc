
add_definitions(-O -Wall -Werror --std=gnu99 -D_GNU_SOURCE -Wno-misleading-indentation)

# The version number.
set(UWSC_VERSION_MAJOR 3)
set(UWSC_VERSION_MINOR 0)
set(UWSC_VERSION_PATCH 0)

# Check the third party Libraries
find_package(Libev REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${LIBEV_INCLUDE_DIR})

set(EXTRA_LIBS ${LIBEV_LIBRARY} dl)
set(SOURCE_FILES uwsc.c log.c utils.c buffer.c sha1.c base64.c ssl.c)

set(UWSC_SSL_SUPPORT_CONFIG 1)
option(UWSC_SSL_SUPPORT "SSL support" ON)

set(UWSC_SSL_NAME OFF)
set(UWSC_HAVE_OPENSSL_CONFIG 0)
set(UWSC_HAVE_CYASSL_CONFIG 0)
set(UWSC_HAVE_MBEDTLS_CONFIG 0)

if(NOT UWSC_SSL_SUPPORT)
    set(UWSC_SSL_SUPPORT_CONFIG 0)
else()
    find_package(OpenSSL)
    find_package(CyaSSL)
    find_package(MbedTLS)

    if(OPENSSL_FOUND)
        set(UWSC_HAVE_OPENSSL_CONFIG 1)
        include_directories(${OPENSSL_INCLUDE_DIR})
        list(APPEND EXTRA_LIBS ${OPENSSL_LIBRARIES})
        set(UWSC_SSL_NAME "OpenSSL")
    elseif(CYASSL_FOUND)
        set(UWSC_HAVE_CYASSL_CONFIG 1)
        include_directories(${CYASSL_INCLUDE_DIR})
        list(APPEND EXTRA_LIBS ${CYASSL_LIBRARIES})
        set(UWSC_SSL_NAME "WolfSSL(CyaSSL)")
    elseif(MBEDTLS_FOUND)
        set(UWSC_HAVE_MBEDTLS_CONFIG 1)
        include_directories(${MBEDTLS_INCLUDE_DIR})
        list(APPEND EXTRA_LIBS ${MBEDTLS_LIBRARIES})
        set(UWSC_SSL_NAME "MbedTLS(PolarSSL)")
    else()
        set(UWSC_SSL_SUPPORT OFF)
        set(UWSC_SSL_SUPPORT_CONFIG 0)
        message(WARNING "No available SSL libraries found")
    endif()

    if(UWSC_SSL_NAME)
        message(STATUS "Select ${UWSC_SSL_NAME} as the SSL backend")
    endif()
endif()

add_library(uwsc SHARED ${SOURCE_FILES})
set_target_properties(uwsc PROPERTIES VERSION ${UWSC_VERSION_MAJOR}.${UWSC_VERSION_MINOR}.${UWSC_VERSION_PATCH})
target_link_libraries(uwsc ${EXTRA_LIBS})

# configure a header file to pass some of the CMake settings to the source code
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

install(
    FILES
	    ${CMAKE_CURRENT_BINARY_DIR}/config.h
        ${CMAKE_CURRENT_SOURCE_DIR}/log.h
	    ${CMAKE_CURRENT_SOURCE_DIR}/uwsc.h
        ${CMAKE_CURRENT_SOURCE_DIR}/buffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ssl.h
    DESTINATION
    	include/uwsc
)

install(
    TARGETS uwsc LIBRARY
    DESTINATION lib
)

message(STATUS "UWSC_VERSION: ${UWSC_VERSION_MAJOR}.${UWSC_VERSION_MINOR}.${UWSC_VERSION_PATCH}")
message(STATUS "UWSC_SSL_SUPPORT: ${UWSC_SSL_NAME}")
