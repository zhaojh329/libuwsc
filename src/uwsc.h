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

#ifndef _UWSC_H
#define _UWSC_H

#include <ev.h>

#include "log.h"
#include "config.h"
#include "buffer.h"

#define UWSC_MAX_CONNECT_TIME       5  /* second */

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
    void *ext;              /* User data */

    void (*onopen)(struct uwsc_client *cl);
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
