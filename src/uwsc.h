/*
 * Copyright (C) 2017 Jianhui Zhao <jianhuizhao329@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UWSC_H
#define _UWSC_H

#include <libubox/uloop.h>
#include <libubox/ustream.h>

#include "config.h"
#include "log.h"

#define UWSC_PING_INTERVAL  30

enum uwsc_error_code {
    UWSC_ERROR_WRITE,
    UWSC_ERROR_INVALID_HEADER,
    UWSC_ERROR_NOT_SUPPORT_FREGMENT,
    UWSC_ERROR_SSL,
    UWSC_ERROR_SSL_INVALID_CERT
};

enum client_state {
    CLIENT_STATE_INIT,
    CLIENT_STATE_HANDSHAKE,
    CLIENT_STATE_MESSAGE
};

enum websocket_op {
    WEBSOCKET_OP_CONTINUE = 0x0,
    WEBSOCKET_OP_TEXT = 0x1,
    WEBSOCKET_OP_BINARY = 0x2,
    WEBSOCKET_OP_CLOSE = 0x8,
    WEBSOCKET_OP_PING = 0x9,
    WEBSOCKET_OP_PONG = 0xA
};

struct uwsc_frame {
    uint8_t fin;
    uint8_t opcode;
    uint64_t payload_len;
    char *payload;
};

struct uwsc_client {
    struct ustream *us;
    struct ustream_fd sfd;
    enum client_state state;
    struct uwsc_frame frame;
    struct uloop_timeout timeout;
    enum uwsc_error_code error;
    
#if (UWSC_SSL_SUPPORT)
    struct ustream_ssl ussl;
    struct ustream_ssl_ctx *ssl_ctx;
    const struct ustream_ssl_ops *ssl_ops;
#endif

    void (*onopen)(struct uwsc_client *cl);
    void (*onmessage)(struct uwsc_client *cl, char *data, uint64_t len, enum websocket_op op);
    void (*onerror)(struct uwsc_client *cl);
    void (*onclose)(struct uwsc_client *cl);
    int (*send)(struct uwsc_client *cl, char *data, int len, enum websocket_op op);
    void (*ping)(struct uwsc_client *cl);
    void (*free)(struct uwsc_client *cl);
};

struct uwsc_client *uwsc_new(const char *url);

#endif
