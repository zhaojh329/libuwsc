find_package(Lua)

set(UWSC_LUA_SUPPORT_DEFAULT "ON")

if (NOT LUA_FOUND)
	set(UWSC_LUA_SUPPORT_DEFAULT "OFF")
endif (NOT LUA_FOUND)

option(UWSC_LUA_SUPPORT "Enable build of Lua module" ${UWSC_LUA_SUPPORT_DEFAULT})

if(UWSC_LUA_SUPPORT)
	add_library(uwsc-lua MODULE uwsc_lua.c)
	target_include_directories(uwsc-lua PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/buffer
        ${CMAKE_SOURCE_DIR}/src/log
        ${CMAKE_BINARY_DIR}/src
        ${LUA_INCLUDE_DIR})
		target_link_libraries(uwsc-lua PRIVATE uwsc)
		set_target_properties(uwsc-lua PROPERTIES OUTPUT_NAME uwsc PREFIX "")

	install(TARGETS uwsc-lua
		LIBRARY DESTINATION lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}
	)
endif(UWSC_LUA_SUPPORT)
