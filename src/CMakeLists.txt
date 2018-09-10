
add_definitions(-O -Wall -Werror --std=gnu99 -D_GNU_SOURCE -Wno-misleading-indentation)

# The version number.
set(UWSC_VERSION_MAJOR 3)
set(UWSC_VERSION_MINOR 0)
set(UWSC_VERSION_PATCH 3)

# Check the third party Libraries
find_package(Libev REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${LIBEV_INCLUDE_DIR})

set(EXTRA_LIBS ${LIBEV_LIBRARY} dl)
set(SOURCE_FILES uwsc.c log.c utils.c buffer.c sha1.c base64.c ssl.c)

set(UWSC_SSL_SUPPORT_CONFIG 1)
option(UWSC_SSL_SUPPORT "SSL support" ON)

option(UWSC_USE_OPENSSL "Force select OpenSSL" OFF)
option(UWSC_USE_WOLFSSL "Force select WolfSSL(CyaSSL)" OFF)
option(UWSC_USE_MBEDTLS "Force select MbedTLS(PolarSSL)" OFF)

set(SSL_NAME OFF)
set(UWSC_HAVE_OPENSSL_CONFIG 0)
set(UWSC_HAVE_WOLFSSL_CONFIG 0)
set(UWSC_HAVE_MBEDTLS_CONFIG 0)

if(NOT UWSC_SSL_SUPPORT)
    set(UWSC_SSL_SUPPORT_CONFIG 0)
else()
    find_package(OpenSSL)
    find_package(WolfSSL)
    find_package(MbedTLS)

	if(UWSC_USE_OPENSSL)
		if (NOT OPENSSL_FOUND)
			set(UWSC_SSL_SUPPORT OFF)
			message(WARNING "Force select OpenSSL, but not found it")
		endif()
	elseif(UWSC_USE_WOLFSSL)
		if (NOT WOLFSSL_FOUND)
			set(UWSC_SSL_SUPPORT OFF)
			message(WARNING "Force select WolfSSL(CyaSSL), but not found it")
		endif()
	elseif(UWSC_USE_MBEDTLS)
		if (NOT MBEDTLS_FOUND)
			set(UWSC_SSL_SUPPORT OFF)
			message(WARNING "Force select MbedTLS(PolarSSL), but not found it")
		endif()
	elseif(OPENSSL_FOUND)
		set(UWSC_USE_OPENSSL ON)
	elseif(WOLFSSL_FOUND)
		set(UWSC_USE_WOLFSSL ON)
	elseif(MBEDTLS_FOUND)
		set(UWSC_USE_MBEDTLS ON)
	else()
		set(UWSC_SSL_SUPPORT OFF)
		message(WARNING "No available SSL libraries found")
	endif()

	if(UWSC_USE_OPENSSL)
		set(SSL_NAME "OpenSSL")
		set(SSL_INC ${OPENSSL_INCLUDE_DIR})
		set(SSL_LIB ${OPENSSL_LIBRARIES})
		set(UWSC_HAVE_OPENSSL_CONFIG 1)
	elseif(UWSC_USE_WOLFSSL)
		set(SSL_NAME "WolfSSL(CyaSSL)")
		set(SSL_INC ${WOLFSSL_INCLUDE_DIR})
		set(SSL_LIB ${WOLFSSL_LIBRARIES})
		set(UWSC_HAVE_WOLFSSL_CONFIG 1)
	elseif(UWSC_USE_MBEDTLS)
		set(SSL_NAME "MbedTLS(PolarSSL)")
		set(SSL_INC ${MBEDTLS_INCLUDE_DIR})
		set(SSL_LIB ${MBEDTLS_LIBRARIES})
		set(UWSC_HAVE_MBEDTLS_CONFIG 1)
	endif()

    if(UWSC_SSL_SUPPORT)
	    include_directories(${SSL_INC})
        list(APPEND EXTRA_LIBS ${SSL_LIB})
        message(STATUS "Select ${SSL_NAME} as the SSL backend")
    else()
	    set(SSL_NAME OFF)
	    set(UWSC_SSL_SUPPORT_CONFIG 0)
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
message(STATUS "UWSC_SSL_SUPPORT: ${SSL_NAME}")
