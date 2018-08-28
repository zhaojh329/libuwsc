/*
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

#ifndef _UWSC_H
#define _UWSC_H

#include <ev.h>

#include "log.h"
#include "config.h"
#include "buffer.h"

#define HTTP_HEAD_LIMIT 4096

/* WebSocket close status codes defined in RFC 6455, section 11.7 */
enum {
    UWSC_CLOSE_STATUS_NORMAL                = 1000,
    UWSC_CLOSE_STATUS_GOINGAWAY             = 1001,
    UWSC_CLOSE_STATUS_PROTOCOL_ERR          = 1002,
    UWSC_CLOSE_STATUS_UNACCEPTABLE_OPCODE   = 1003,
    UWSC_CLOSE_STATUS_RESERVED              = 1004,
    UWSC_CLOSE_STATUS_NO_STATUS             = 1005,
    UWSC_CLOSE_STATUS_ABNORMAL_CLOSE        = 1006,
    UWSC_CLOSE_STATUS_INVALID_PAYLOAD       = 1007,
    UWSC_CLOSE_STATUS_POLICY_VIOLATION      = 1008,
    UWSC_CLOSE_STATUS_MESSAGE_TOO_LARGE     = 1009,
    UWSC_CLOSE_STATUS_EXTENSION_REQUIRED    = 1010,
    UWSC_CLOSE_STATUS_UNEXPECTED_CONDITION  = 1011,
    UWSC_CLOSE_STATUS_TLS_FAILURE           = 1015
};

enum {
    UWSC_ERROR_IO = 1,
    UWSC_ERROR_INVALID_HEADER,
    UWSC_ERROR_SERVER_MASKED,
    UWSC_ERROR_NOT_SUPPORT,
    UWSC_ERROR_PING_TIMEOUT,
    UWSC_ERROR_CONNECT,
    UWSC_ERROR_SSL_HANDSHAKE
};

enum {
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_SSL_HANDSHAKE,
    CLIENT_STATE_HANDSHAKE,
    CLIENT_STATE_MESSAGE
};

enum {
    UWSC_OP_CONTINUE   = 0x0,
    UWSC_OP_TEXT       = 0x1,
    UWSC_OP_BINARY     = 0x2,
    UWSC_OP_CLOSE      = 0x8,
    UWSC_OP_PING       = 0x9,
    UWSC_OP_PONG       = 0xA
};

struct uwsc_frame {
    uint8_t opcode;
    size_t payloadlen;
    uint8_t *payload;
};

struct uwsc_client {
    int sock;
    int state;
    struct ev_loop *loop;
    struct ev_io ior;
    struct ev_io iow;
    struct buffer rb;
    struct buffer wb;
    struct uwsc_frame frame;
    struct ev_timer timer;
    bool wait_pong;
    int ping_interval;
    char key[256];  /* Sec-WebSocket-Key */
    void *ssl;

    void (*onopen)(struct uwsc_client *cl);
    void (*set_ping_interval)(struct uwsc_client *cl, int interval);
    void (*onmessage)(struct uwsc_client *cl, void *data, size_t len, bool binary);
    void (*onerror)(struct uwsc_client *cl, int err, const char *msg);
    void (*onclose)(struct uwsc_client *cl, int code, const char *reason);
    int (*send)(struct uwsc_client *cl, const void *data, size_t len, int op);
    void (*ping)(struct uwsc_client *cl);
};

struct uwsc_client *uwsc_new_ssl_v2(const char *url, const char *ca_crt_file, bool verify,
    struct ev_loop *loop);

static inline struct uwsc_client *uwsc_new_ssl(const char *url, const char *ca_crt_file,
    bool verify)
{
    return uwsc_new_ssl_v2(url, ca_crt_file, verify, EV_DEFAULT);
}

static inline struct uwsc_client *uwsc_new(const char *url)
{
    return uwsc_new_ssl(url, NULL, false);
}

static inline struct uwsc_client *uwsc_new_v2(const char *url, struct ev_loop *loop)
{
    return uwsc_new_ssl_v2(url, NULL, false, loop);
}

#endif
