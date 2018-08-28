/*
 * Modified from mongoose(https://github.com/cesanta/mongoose)
 *
 * Copyright (C) 2017 Jianhui Zhao <jianhuizhao329@gmail.com>
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

#ifndef _SHA1_H
#define _SHA1_H

#include <stdint.h>

struct sha1_ctx {
    uint32_t state[5];
    size_t count[2];
    uint8_t buffer[64];
};

void sha1_init(struct sha1_ctx *ctx);
void sha1_update(struct sha1_ctx *ctx, const void *data, size_t len);
void sha1_final(struct sha1_ctx *ctx, uint8_t digest[20]);

#endif