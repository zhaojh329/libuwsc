find_package(Lua)

set(UWSC_LUA_SUPPORT_DEFAULT "ON")

if (NOT LUA_FOUND)
	set(BUILD_LUA_DEFAULT "OFF")
endif (NOT LUA_FOUND)

option(UMQTT_LUA_SUPPORT "Enable build of Lua module" ${UWSC_LUA_SUPPORT_DEFAULT})

if(UMQTT_LUA_SUPPORT)
	include_directories(${LUA_INCLUDE_DIR})

	add_library(uwsc-lua MODULE uwsc_lua.c)
	set_target_properties(uwsc-lua PROPERTIES OUTPUT_NAME uwsc PREFIX "")

	target_link_libraries(uwsc-lua uwsc)

	install(TARGETS uwsc-lua
		LIBRARY DESTINATION lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}
	)
endif(UMQTT_LUA_SUPPORT)
