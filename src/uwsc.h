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

#define HTTP_HEAD_LIMIT             4096
#define UWSC_MAX_CONNECT_TIME       5  /* second */
#define UWSC_BUFFER_PERSISTENT_SIZE 4096

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
    CLIENT_STATE_PARSE_MSG_HEAD,
    CLIENT_STATE_PARSE_MSG_PAYLEN,
    CLIENT_STATE_PARSE_MSG_PAYLOAD
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
    ev_tstamp start_time;   /* Time stamp of begin connect */
    ev_tstamp last_ping;    /* Time stamp of last ping */
    int ntimeout;           /* Number of timeouts */
    char key[256];          /* Sec-WebSocket-Key */
    void *ssl;              /* Context wrap of openssl, wolfssl and mbedtls */

    void (*onopen)(struct uwsc_client *cl);
    void (*set_ping_interval)(struct uwsc_client *cl, int interval);
    void (*onmessage)(struct uwsc_client *cl, void *data, size_t len, bool binary);
    void (*onerror)(struct uwsc_client *cl, int err, const char *msg);
    void (*onclose)(struct uwsc_client *cl, int code, const char *reason);

    int (*send)(struct uwsc_client *cl, const void *data, size_t len, int op);
    int (*send_ex)(struct uwsc_client *cl, int op, int num, ...);
    int (*send_close)(struct uwsc_client *cl, int code, const char *reason);
    void (*ping)(struct uwsc_client *cl);
    void (*free)(struct uwsc_client *cl);
};

/*
 *  uwsc_new - creat an uwsc_client struct and connect to server
 *  @loop: If NULL will use EV_DEFAULT
 *  @url: A websock url. ws://xxx.com/xx or wss://xxx.com/xx
 *  @ping_interval: ping interval
 *  @extra_header: extra http header. Authorization: a1d4cdb1a3cd6a0e94aa3599afcddcf5\r\n
 */
struct uwsc_client *uwsc_new(struct ev_loop *loop, const char *url,
    int ping_interval, const char *extra_header);

int uwsc_init(struct uwsc_client *cl, struct ev_loop *loop, const char *url,
    int ping_interval, const char *extra_header);

#endif
