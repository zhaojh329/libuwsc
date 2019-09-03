/*
 * MIT License
 *
 * Copyright (c) 2019 Jianhui Zhao <zhaojh329@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "uwsc_lua.h"

#define UWSC_MT "uqmtt"

/* https://github.com/brimworks/lua-ev/blob/master/lua_ev.h#L33 */
#define EV_LOOP_MT    "ev{loop}"
#define EV_UNINITIALIZED_DEFAULT_LOOP (struct ev_loop *)1

static int uwsc_lua_version(lua_State *L)
{
    lua_pushinteger(L, UWSC_VERSION_MAJOR);
    lua_pushinteger(L, UWSC_VERSION_MINOR);
    lua_pushinteger(L, UWSC_VERSION_PATCH);

    return 3;
}

static void uwsc_onmessage(struct uwsc_client *cli, void *data, size_t len, bool binary)
{
    struct uwsc_client_lua *cl = container_of(cli, struct uwsc_client_lua, cli);
    lua_State *L = cl->L;

    lua_rawgeti(L, LUA_REGISTRYINDEX, cl->onmessage_ref);
	if (!lua_isfunction(L, -1))
		return;
    
    lua_pushlstring(L, data, len);
    lua_pushboolean(L, binary);

    lua_call(L, 2, 0);
}

static void uwsc_onerror(struct uwsc_client *cli, int err, const char *msg)
{
    struct uwsc_client_lua *cl = container_of(cli, struct uwsc_client_lua, cli);
    lua_State *L = cl->L;

    cl->connected = false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, cl->onerror_ref);
	if (!lua_isfunction(L, -1))
		return;

    lua_pushinteger(L, err);
    lua_pushstring(L, msg);

    lua_call(L, 2, 0);
}

static void uwsc_onclose(struct uwsc_client *cli, int code, const char *reason)
{
    struct uwsc_client_lua *cl = container_of(cli, struct uwsc_client_lua, cli);
    lua_State *L = cl->L;

    cl->connected = false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, cl->onclose_ref);
	if (!lua_isfunction(L, -1))
		return;

    lua_pushinteger(L, code);
    lua_pushstring(L, reason);
    
    lua_call(L, 2, 0);
}

static void uwsc_onopen(struct uwsc_client *cli)
{
    struct uwsc_client_lua *cl = container_of(cli, struct uwsc_client_lua, cli);
    lua_State *L = cl->L;

    cl->connected = true;

    lua_rawgeti(L, LUA_REGISTRYINDEX, cl->onopen_ref);
	if (!lua_isfunction(L, -1))
		return;

    lua_call(L, 0, 0);
}

static int uwsc_lua_new(lua_State *L)
{
    struct ev_loop *loop = NULL;
    struct uwsc_client_lua *cl;
    const char *url = lua_tostring(L, 1);
    int ping_interval = lua_tointeger(L, 2);
    const char *extra_header = NULL;
    
    if (lua_istable(L, 3)) {
        struct ev_loop **tmp;

        lua_getfield(L, 3, "loop");
        if (!lua_isnil(L, -1)) {
            tmp = luaL_checkudata(L, 2, EV_LOOP_MT);
            if (*tmp != EV_UNINITIALIZED_DEFAULT_LOOP)
                loop = *tmp;
        }

        lua_getfield(L, 3, "extra_header");
        extra_header = lua_tostring(L, -1);
    }

    cl = lua_newuserdata(L, sizeof(struct uwsc_client_lua));
    if (!cl) {
        lua_pushnil(L);
        lua_pushstring(L, "lua_newuserdata() failed");
        return 2;
    }

	memset(cl, 0, sizeof(struct uwsc_client_lua));

	luaL_getmetatable(L, UWSC_MT);
	lua_setmetatable(L, -2);

    if (uwsc_init(&cl->cli, loop, url, ping_interval, extra_header) < 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "uwsc_init() failed");
        return 2;
    }

    cl->L = L;
    cl->cli.onopen = uwsc_onopen;
    cl->cli.onmessage = uwsc_onmessage;
    cl->cli.onerror = uwsc_onerror;
    cl->cli.onclose = uwsc_onclose;

	return 1;
}

static int uwsc_lua_on(lua_State *L)
{
    struct uwsc_client_lua *cl = luaL_checkudata(L, 1, UWSC_MT);
    const char *name = luaL_checkstring(L, 2);
    int ref;

    luaL_checktype(L, 3, LUA_TFUNCTION);

    ref = luaL_ref(L, LUA_REGISTRYINDEX);

    if (!strcmp(name, "open"))
        cl->onopen_ref = ref;
    else if (!strcmp(name, "message"))
        cl->onmessage_ref = ref;
    else if (!strcmp(name, "error"))
        cl->onerror_ref = ref;
    else if (!strcmp(name, "close"))
        cl->onclose_ref = ref;
    else
        luaL_argcheck(L, false, 2, "available event name: open message error close");

	return 0;
}

static int __uwsc_lua_send(lua_State *L, int op)
{
    struct uwsc_client_lua *cl = luaL_checkudata(L, 1, UWSC_MT);
    size_t len;
    const void *data = luaL_checklstring(L, 2, &len);

    cl->cli.send(&cl->cli, data, len,  op);

    return 0;
}

static int uwsc_lua_send(lua_State *L)
{
    int op = lua_tointeger(L, 3);

    return __uwsc_lua_send(L, op);
}

static int uwsc_lua_send_text(lua_State *L)
{
    return __uwsc_lua_send(L, UWSC_OP_TEXT);
}

static int uwsc_lua_send_binary(lua_State *L)
{
    return __uwsc_lua_send(L, UWSC_OP_BINARY);
}

static int uwsc_lua_gc(lua_State *L)
{
	struct uwsc_client_lua *cl = luaL_checkudata(L, 1, UWSC_MT);

    if (cl->connected) {
        cl->cli.send_close(&cl->cli, UWSC_CLOSE_STATUS_NORMAL, "");
        cl->cli.free(&cl->cli);
        cl->connected = false;
    }

    return 0;
}

static const luaL_Reg uwsc_meta[] = {
    {"on", uwsc_lua_on},
    {"send", uwsc_lua_send},
    {"send_text", uwsc_lua_send_text},
    {"send_binary", uwsc_lua_send_binary},
    {"__gc", uwsc_lua_gc},
	{NULL, NULL}
};

static const luaL_Reg uwsc_fun[] = {
    {"new", uwsc_lua_new},
    {"version", uwsc_lua_version},
    {NULL, NULL}
};

int luaopen_uwsc(lua_State *L)
{
    /* metatable.__index = metatable */
    luaL_newmetatable(L, UWSC_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, uwsc_meta, 0);

    lua_newtable(L);
    luaL_setfuncs(L, uwsc_fun, 0);

    return 1;
}
