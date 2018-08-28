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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "ssl.h"
#include "uwsc.h"
#include "sha1.h"
#include "utils.h"
#include "base64.h"

static void uwsc_free(struct uwsc_client *cl)
{
    ev_timer_stop(cl->loop, &cl->timer);
    ev_io_stop(cl->loop, &cl->ior);
    ev_io_stop(cl->loop, &cl->iow);
    buffer_free(&cl->rb);
    buffer_free(&cl->wb);

#if UWSC_SSL_SUPPORT
    uwsc_ssl_free(cl->ssl);
#endif

    if (cl->sock > 0)
        close(cl->sock);
}

static inline void uwsc_error(struct uwsc_client *cl, int err, const char *msg)
{
    uwsc_free(cl);

    if (cl->onerror) {
        if (!msg)
            msg = "";
        cl->onerror(cl, err, msg);
    }
}

static int uwsc_send_close(struct uwsc_client *cl, int code, const char *reason)
{
    char buf[128] = "";

    buf[1] = code & 0xFF;
    buf[0] = (code >> 8)& 0xFF;

    strncpy(&buf[2], reason, sizeof(buf) - 3);

    return cl->send(cl, buf, strlen(buf + 2) + 2, UWSC_OP_CLOSE);
}

static void dispach_message(struct uwsc_client *cl)
{
    struct uwsc_frame *frame = &cl->frame;

    switch (frame->opcode) {
    case UWSC_OP_TEXT:
    case UWSC_OP_BINARY:
        if (cl->onmessage)
            cl->onmessage(cl, frame->payload, frame->payloadlen, frame->opcode == UWSC_OP_BINARY);
        break;

    case UWSC_OP_PING:
        cl->send(cl, frame->payload, frame->payloadlen, UWSC_OP_PONG);
        break;

    case UWSC_OP_PONG:
        cl->wait_pong = false;
        break;

    case UWSC_OP_CLOSE:
            if (cl->onclose) {
                int code = buffer_pull_u16(&cl->rb);
                char reason[128] = "";

                frame->payloadlen -= 2;
                buffer_pull(&cl->rb, reason, frame->payloadlen);
                cl->onclose(cl, ntohs(code), reason);
            }

            uwsc_free(cl);
        break;

    default:
        uwsc_log_err("unknown opcode - %d\n", frame->opcode);
        uwsc_send_close(cl, UWSC_CLOSE_STATUS_PROTOCOL_ERR, "unknown opcode");
        break;
    }

    if (frame->payloadlen > 0) {
        buffer_pull(&cl->rb, NULL, frame->payloadlen);
        frame->payloadlen = 0;
    }
}

static bool parse_header_len(struct uwsc_client *cl, uint64_t *payloadlen,
    int *payloadlen_size)
{
    struct uwsc_frame *frame = &cl->frame;
    struct buffer *rb = &cl->rb;
    uint8_t *data = buffer_data(rb);
    bool fin;

    if (buffer_length(rb) < 2)
        return false;

    fin = (data[0] & 0x80) ? true : false;
    frame->opcode = data[0] & 0x0F;

    if (!fin || frame->opcode == UWSC_OP_CONTINUE) {
        uwsc_error(cl, UWSC_ERROR_NOT_SUPPORT, "Not support fragment");
        return false;
    }

    if (data[1] & 0x80) {
        uwsc_error(cl, UWSC_ERROR_SERVER_MASKED, "Masked error");
        return false;
    }

    *payloadlen_size = 1;
    *payloadlen = data[1] & 0x7F;

    switch (*payloadlen) {
    case 126:
        if (buffer_length(rb) < 4)
            return false;
        *payloadlen = ntohs(*(uint16_t *)&data[2]);
        *payloadlen_size += 2;
        break;
    case 127:
        uwsc_error(cl, UWSC_ERROR_NOT_SUPPORT, "Payload too large");
        uwsc_send_close(cl, UWSC_CLOSE_STATUS_MESSAGE_TOO_LARGE, "");
        break;
    default:
        break;
    }

    return true;
}

static bool parse_frame(struct uwsc_client *cl)
{
    struct uwsc_frame *frame = &cl->frame;
    struct buffer *rb = &cl->rb;
    uint64_t payloadlen;
    int payloadlen_size;

    if (!parse_header_len(cl, &payloadlen, &payloadlen_size))
        return false;

    if (buffer_length(rb) < 1 + payloadlen_size + payloadlen)
        return false;

    frame->payloadlen = payloadlen;

    buffer_pull(rb, NULL, payloadlen_size + 1);

    frame->payload = buffer_data(rb);

    dispach_message(cl);

    return true;
}

static int parse_http_header(struct uwsc_client *cl)
{
    char *k, *v;
    bool has_upgrade = false;
    bool has_connection = false;
    bool has_sec_webSocket_accept = false;

    while (1) {
        k = strtok(NULL, "\r\n");
        if (!k)
            break;
        
        v = strchr(k, ':');
        if (!v)
            break;

        *v++ = 0;

        while (*v == ' ')
            v++;

        if (!strcasecmp(k, "Upgrade") && !strcasecmp(v, "websocket"))
            has_upgrade = true;

        if (!strcasecmp(k, "Connection") && !strcasecmp(v, "upgrade"))
            has_connection = true;

        if (!strcasecmp(k, "Sec-WebSocket-Accept")) {
            static const char *magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            struct sha1_ctx ctx;
            unsigned char sha[20];
            char my[512] = "";

            sha1_init(&ctx);
            sha1_update(&ctx, (uint8_t *)cl->key, strlen(cl->key));
            sha1_update(&ctx, (uint8_t *)magic, strlen(magic));
            sha1_final(&ctx, sha);

            b64_encode(sha, sizeof(sha), my, sizeof(my));

            /* verify the value of Sec-WebSocket-Accept */
            if (strcmp(v, my)) {
                uwsc_log_err("verify Sec-WebSocket-Accept failed\n");
                return -1;
            }
            has_sec_webSocket_accept = true;
        }
    }

    if (!has_upgrade || !has_connection || !has_sec_webSocket_accept)
        return -1;
    
    return 0;
}

static void uwsc_parse(struct uwsc_client *cl)
{
    struct buffer *rb = &cl->rb;
    int err = 0;

    do {
        int data_len = buffer_length(rb);
        if (data_len == 0)
            return;

        if (cl->state == CLIENT_STATE_HANDSHAKE) {
            char *version, *status_code, *summary;
            char *p, *data = buffer_data(rb);

            p = memmem(data, data_len, "\r\n\r\n", 4);
            if (!p)
                return;
            p[0] = '\0';

            version = strtok(data, " ");
            status_code = strtok(NULL, " ");
            summary = strtok(NULL, "\r\n");

            if (!version || strcmp(version, "HTTP/1.1")) {
                err = UWSC_ERROR_INVALID_HEADER;
                break;
            }

            if (!status_code || atoi(status_code) != 101) {
                err = UWSC_ERROR_INVALID_HEADER;
                break;
            }

            if (!summary) {
                err = UWSC_ERROR_INVALID_HEADER;
                break;
            }

            if (parse_http_header(cl)) {
                err = UWSC_ERROR_INVALID_HEADER;
                break;
            }

            buffer_pull(rb, NULL, p - data + 4);

            if (cl->onopen)
                cl->onopen(cl);

            cl->state = CLIENT_STATE_MESSAGE;
        } else if (cl->state == CLIENT_STATE_MESSAGE) {
            if (!parse_frame(cl))
                break;
        } else {
            uwsc_log_err("Invalid state\n");
        }
       
    } while(!err);

    if (err)
        uwsc_error(cl, err, "Invalid header");
}

static void uwsc_io_read_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
    struct uwsc_client *cl = container_of(w, struct uwsc_client, ior);
    struct buffer *rb = &cl->rb;
    bool eof;
    int ret;

    if (cl->state == CLIENT_STATE_CONNECTING) {
        int err;
        socklen_t optlen = sizeof(err);

        getsockopt(w->fd, SOL_SOCKET, SO_ERROR, &err, &optlen);
        if (err) {
            uwsc_error(cl, UWSC_ERROR_CONNECT, strerror(err));
            return;
        }
        cl->state = CLIENT_STATE_HANDSHAKE;
        return;
    }

#if UWSC_SSL_SUPPORT
    if (cl->ssl)
        ret = buffer_put_fd(rb, w->fd, -1, &eof, uwsc_ssl_read, cl->ssl);
    else
#endif
        ret = buffer_put_fd(rb, w->fd, -1, &eof, NULL, NULL);

    if (ret < 0) {
        uwsc_error(cl, UWSC_ERROR_IO, "read error");
        return;
    }

    if (eof) {
        uwsc_free(cl);

        if (cl->onclose)
            cl->onclose(cl, UWSC_CLOSE_STATUS_ABNORMAL_CLOSE, "unexpected EOF");
        return;
    }

    uwsc_parse(cl);
}

static void uwsc_io_write_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
    struct uwsc_client *cl = container_of(w, struct uwsc_client, iow);
    int ret;

    if (unlikely(cl->state == CLIENT_STATE_CONNECTING)) {
#if UWSC_SSL_SUPPORT
        if (cl->ssl)
            cl->state = CLIENT_STATE_SSL_HANDSHAKE;
        else
#endif
            cl->state = CLIENT_STATE_HANDSHAKE;
    }

#if UWSC_SSL_SUPPORT
    if (unlikely(cl->state == CLIENT_STATE_SSL_HANDSHAKE)) {
        ret = uwsc_ssl_handshake(cl->ssl);
        if (ret == 1)
            cl->state = CLIENT_STATE_HANDSHAKE;
        else if (ret == -1)
            uwsc_error(cl, UWSC_ERROR_SSL_HANDSHAKE, "ssl handshake failed");
        return;
    }
#endif

#if UWSC_SSL_SUPPORT
    if (cl->ssl)
        ret = buffer_pull_to_fd(&cl->wb, w->fd, buffer_length(&cl->wb), uwsc_ssl_write, cl->ssl);
    else
#endif
        ret = buffer_pull_to_fd(&cl->wb, w->fd, buffer_length(&cl->wb), NULL, NULL);

    if (ret < 0) {
        uwsc_error(cl, UWSC_ERROR_IO, "write error");
        return;
    }

    if (buffer_length(&cl->wb) < 1)
        ev_io_stop(loop, w);
}

static int uwsc_send(struct uwsc_client *cl, const void *data, size_t len, int op)
{
    struct buffer *wb = &cl->wb;
    const uint8_t *p;
    uint8_t mk[4];
    int i;

    get_nonce(mk, 4);

    buffer_put_u8(wb, 0x80 | op);

    if (len < 126) {
        buffer_put_u8(wb, 0x80 | len);
    } else if (len < 65536) {
        buffer_put_u8(wb, 0x80 | 126);
        buffer_put_u8(wb, (len >> 8) & 0xFF);
        buffer_put_u8(wb, len & 0xFF);
    } else {
        uwsc_log_err("Payload too large");
        return -1;
    }

    buffer_put_data(wb, mk, 4);

    p = data;
    for (i = 0; i < len; i++)
        buffer_put_u8(wb, p[i] ^ mk[i % 4]);

    ev_io_start(cl->loop, &cl->iow);

    return 0;
}

static inline void uwsc_ping(struct uwsc_client *cl)
{
    const char *msg = "libuwsc";
    cl->send(cl, msg, strlen(msg), UWSC_OP_PING);
}

static void uwsc_handshake(struct uwsc_client *cl, const char *host, int port, const char *path)
{
    struct buffer *wb = &cl->wb;
    uint8_t nonce[16];

    get_nonce(nonce, sizeof(nonce));
    
    b64_encode(nonce, sizeof(nonce), cl->key, sizeof(cl->key));

    buffer_put_printf(wb, "GET %s HTTP/1.1\r\n", path);
    buffer_put_string(wb, "Upgrade: websocket\r\n");
    buffer_put_string(wb, "Connection: Upgrade\r\n");
    buffer_put_printf(wb, "Sec-WebSocket-Key: %s\r\n", cl->key);
    buffer_put_string(wb, "Sec-WebSocket-Version: 13\r\n");

    buffer_put_printf(wb, "Host: %s", host);
    if (port == 80)
        buffer_put_string(wb, "\r\n");
    else
        buffer_put_printf(wb, ":%d\r\n", port);
    
    buffer_put_string(wb, "\r\n");

    ev_io_start(cl->loop, &cl->iow);
}

static void uwsc_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    struct uwsc_client *cl = container_of(w, struct uwsc_client, timer);
    static time_t connect_time;
    static time_t last_ping;
    static int ntimeout;
    time_t now = time(NULL);

    if (unlikely(cl->state == CLIENT_STATE_CONNECTING)) {
        if (connect_time == 0) {
            connect_time = now;
            return;
        }

        if (now - connect_time > 5) {
            uwsc_error(cl, UWSC_ERROR_CONNECT, "Connect timeout");
            return;
        }
    }

    if (unlikely(cl->state != CLIENT_STATE_MESSAGE))
        return;

    if (cl->ping_interval == 0)
        return;

    if (unlikely(cl->wait_pong)) {
        if (now - last_ping < 3)
            return;

        uwsc_log_err("ping timeout %d\n", ++ntimeout);
        if (ntimeout > 2) {
            uwsc_error(cl, UWSC_ERROR_PING_TIMEOUT, "ping timeout");
            return;
        }
    } else {
        ntimeout = 0;
    }


    if (now - last_ping < cl->ping_interval)
        return;
    last_ping = now;

    cl->ping(cl);
    cl->wait_pong = true;
}

static void uwsc_set_ping_interval(struct uwsc_client *cl, int interval)
{
    cl->ping_interval = interval;
}

struct uwsc_client *uwsc_new_ssl_v2(const char *url, const char *ca_crt_file,
    bool verify, struct ev_loop *loop)
{
    struct uwsc_client *cl = NULL;
    const char *path = "/";
    char host[256] = "";
    bool inprogress;
    int sock = -1;
    int port;
    bool ssl;
    int eai;

    if (parse_url(url, host, sizeof(host), &port, &path, &ssl) < 0) {
        uwsc_log_err("Invalid url\n");
        return NULL;
    }

    sock = tcp_connect(host, port, SOCK_NONBLOCK | SOCK_CLOEXEC, &inprogress, &eai);
    if (sock < 0) {
        uwsc_log_err("tcp_connect failed: %s\n", strerror(errno));
        return NULL;
    } else if (sock == 0) {
        uwsc_log_err("tcp_connect failed: %s\n", gai_strerror(eai));
        return NULL;
    }

    cl = calloc(1, sizeof(struct uwsc_client));
    if (!cl) {
        uwsc_log_err("calloc failed: %s\n", strerror(errno));
        goto err;
    }

    if (!inprogress)
        cl->state = CLIENT_STATE_HANDSHAKE;

    cl->loop = loop;
    cl->sock = sock;
    cl->send = uwsc_send;
    cl->ping = uwsc_ping;
    cl->set_ping_interval = uwsc_set_ping_interval;

    if (ssl) {
#if (UWSC_SSL_SUPPORT)
        uwsc_ssl_init((struct uwsc_ssl_ctx **)&cl->ssl, cl->sock);
#else
        uwsc_log_err("SSL is not enabled at compile\n");
        goto err;
#endif
    }

    ev_io_init(&cl->iow, uwsc_io_write_cb, sock, EV_WRITE);

    ev_io_init(&cl->ior, uwsc_io_read_cb, sock, EV_READ);
    ev_io_start(loop, &cl->ior);

    ev_timer_init(&cl->timer, uwsc_timer_cb, 0.0, 1.0);
    ev_timer_start(cl->loop, &cl->timer);

    uwsc_handshake(cl, host, port, path);
    
    return cl;

err:
    if (sock > 0)
        close(sock);

    if (cl)
        free(cl);

    return NULL;
}
