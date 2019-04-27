/*
 * Copyright (C) 2019 Jianhui Zhao <jianhuizhao329@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef __UWSC_LUA_H
#define __UWSC_LUA_H

#include <lauxlib.h>
#include <lualib.h>

#include "uwsc.h"

/* Compatibility defines */
#if LUA_VERSION_NUM <= 501

/* NOTE: this only works if nups == 0! */
#define luaL_setfuncs(L, fns, nups) luaL_register((L), NULL, (fns))

#endif

struct uwsc_client_lua {
    lua_State *L;

    struct uwsc_client cli;
    bool connected;

    int onopen_ref;
    int onmessage_ref;
    int onerror_ref;
    int onclose_ref;
};

#endif