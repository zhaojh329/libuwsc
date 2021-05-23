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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <netdb.h>
#include <limits.h>

#include "uwsc.h"
#include "sha1.h"
#include "utils.h"

#ifdef SSL_SUPPORT
#include "ssl/ssl.h"
#endif

#ifdef SSL_SUPPORT
static struct ssl_context *ssl_ctx;
#endif

static void uwsc_free(struct uwsc_client *cl)
{
    ev_timer_stop(cl->loop, &cl->timer);
    ev_io_stop(cl->loop, &cl->ior);
    ev_io_stop(cl->loop, &cl->iow);
    buffer_free(&cl->rb);
    buffer_free(&cl->wb);

#ifdef SSL_SUPPORT
    ssl_session_free(cl->ssl);
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

    code = htobe16(code & 0xFFFF);
    memcpy(buf, &code, 2);

    if (reason)
        strncpy(&buf[2], reason, sizeof(buf) - 3);

    return cl->send(cl, buf, strlen(buf + 2) + 2, UWSC_OP_CLOSE);
}

static bool parse_header(struct uwsc_client *cl)
{
    struct uwsc_frame *frame = &cl->frame;
    struct buffer *rb = &cl->rb;
    uint8_t head, len;
    bool fin;

    if (buffer_length(rb) < 2)
        return false;

    head = buffer_pull_u8(rb);

    fin = (head & 0x80) ? true : false;
    frame->opcode = head & 0x0F;

    if (!fin || frame->opcode == UWSC_OP_CONTINUE) {
        uwsc_error(cl, UWSC_ERROR_NOT_SUPPORT, "Not support fragment");
        return false;
    }

    len = buffer_pull_u8(rb);
    if (len & 0x80) {
        uwsc_error(cl, UWSC_ERROR_SERVER_MASKED, "Masked error");
        return false;
    }

    frame->payloadlen = len & 0x7F;

    cl->state = CLIENT_STATE_PARSE_MSG_PAYLEN;
    return true;
}

static bool parse_paylen(struct uwsc_client *cl)
{
    struct uwsc_frame *frame = &cl->frame;
    struct buffer *rb = &cl->rb;
    uint64_t len;

    switch (frame->payloadlen) {
    case 126:
        if (buffer_length(rb) < 4)
            return false;
        frame->payloadlen = be16toh(buffer_pull_u16(rb));
        break;
    case 127:
        if (buffer_length(rb) < 10)
            return false;
        len = be64toh(buffer_pull_u64(rb));
        if (len > ULONG_MAX) {
            uwsc_error(cl, UWSC_ERROR_NOT_SUPPORT, "Payload too large");
            uwsc_send_close(cl, UWSC_CLOSE_STATUS_MESSAGE_TOO_LARGE, "");
        } else {
            frame->payloadlen = len;
        }
        break;
    default:
        break;
    }

    cl->state = CLIENT_STATE_PARSE_MSG_PAYLOAD;

    return true;
}

static bool dispach_message(struct uwsc_client *cl)
{
    struct buffer *rb = &cl->rb;
    struct uwsc_frame *frame = &cl->frame;
    uint8_t *payload = buffer_data(rb);

    if (buffer_length(rb) < frame->payloadlen)
        return false;

    switch (frame->opcode) {
    case UWSC_OP_TEXT:
    case UWSC_OP_BINARY:
        if (cl->onmessage)
            cl->onmessage(cl, payload, frame->payloadlen, frame->opcode == UWSC_OP_BINARY);
        break;

    case UWSC_OP_PING:
        cl->send(cl, payload, frame->payloadlen, UWSC_OP_PONG);
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
        log_err("unknown opcode - %d\n", frame->opcode);
        uwsc_send_close(cl, UWSC_CLOSE_STATUS_PROTOCOL_ERR, "unknown opcode");
        break;
    }

    buffer_pull(&cl->rb, NULL, frame->payloadlen);
    cl->state = CLIENT_STATE_PARSE_MSG_HEAD;

    return true;
}

static bool parse_frame(struct uwsc_client *cl)
{
    switch (cl->state) {
    case CLIENT_STATE_PARSE_MSG_HEAD:
        if (!parse_header(cl))
            return false;
    case CLIENT_STATE_PARSE_MSG_PAYLEN:
        if (!parse_paylen(cl))
            return false;
    case CLIENT_STATE_PARSE_MSG_PAYLOAD:
        if (!dispach_message(cl))
            return false;
    default:
        break;
    }

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
                log_err("verify Sec-WebSocket-Accept failed\n");
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

        if (unlikely(cl->state < CLIENT_STATE_PARSE_MSG_HEAD)) {
            char *version, *status_code;
            char *p, *data = buffer_data(rb);

            p = memmem(data, data_len, "\r\n\r\n", 4);
            if (!p)
                return;
            p[0] = '\0';

            version = strtok(data, " ");
            status_code = strtok(NULL, "\r\n");

            if (!version || strcmp(version, "HTTP/1.1")) {
                err = UWSC_ERROR_INVALID_HEADER;
                break;
            }

            if (!status_code || atoi(status_code) != 101) {
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

            cl->state = CLIENT_STATE_PARSE_MSG_HEAD;
        } else {
            if (!parse_frame(cl))
                break;
        }
       
    } while(!err);

    if (err)
        uwsc_error(cl, err, "Invalid header");
}

static int check_socket_state(struct uwsc_client *cl)
{
    int err;
    socklen_t optlen = sizeof(err);

    getsockopt(cl->sock, SOL_SOCKET, SO_ERROR, &err, &optlen);
    if (err) {
        uwsc_error(cl, UWSC_ERROR_CONNECT, strerror(err));
        return -1;
    }

#ifdef SSL_SUPPORT
    if (cl->ssl)
        cl->state = CLIENT_STATE_SSL_HANDSHAKE;
    else
#endif
        cl->state = CLIENT_STATE_HANDSHAKE;

    return 0;
}

#ifdef SSL_SUPPORT
static void on_ssl_verify_error(int error, const char *str, void *arg)
{
    log_warn("SSL certificate error(%d): %s\n", error, str);
}

/* -1 error, 0 pending, 1 ok */
static int ssl_negotiated(struct uwsc_client *cl)
{
    char err_buf[128];
    int ret;

    ret = ssl_connect(cl->ssl, false, on_ssl_verify_error, NULL);
    if (ret == SSL_PENDING)
        return 0;

    if (ret == SSL_ERROR) {
        log_err("ssl connect error(%d): %s\n", ssl_err_code, ssl_strerror(ssl_err_code, err_buf, sizeof(err_buf)));
        uwsc_error(cl, UWSC_ERROR_SSL_HANDSHAKE, err_buf);
        return -1;
    }

    cl->state = CLIENT_STATE_HANDSHAKE;

    return 1;
}

static int uwsc_ssl_read(int fd, void *buf, size_t count, void *arg)
{
    struct uwsc_client *cl = arg;
    static char err_buf[128];
    int ret;

    ret = ssl_read(cl->ssl, buf, count);
    if (ret == SSL_ERROR) {
        log_err("ssl_read(%d): %s\n", ssl_err_code,
                ssl_strerror(ssl_err_code, err_buf, sizeof(err_buf)));
        uwsc_error(cl, UWSC_ERROR_IO, err_buf);
        return P_FD_ERR;
    }

    if (ret == SSL_PENDING)
        return P_FD_PENDING;

    return ret;
}
#endif

static void uwsc_io_read_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
    struct uwsc_client *cl = container_of(w, struct uwsc_client, ior);
    struct buffer *rb = &cl->rb;
    bool eof;
    int ret;

    if (cl->state == CLIENT_STATE_CONNECTING) {
        if (check_socket_state(cl) < 0)
            return;
    }

    if (cl->ssl) {
#ifdef SSL_SUPPORT
        if (unlikely(cl->state == CLIENT_STATE_SSL_HANDSHAKE)) {
            ret = ssl_negotiated(cl);
            if (ret <= 0)
                return;
        }

        ret = buffer_put_fd_ex(rb, w->fd, 4096, &eof, uwsc_ssl_read, cl);
        if (ret < 0)
            return;
#endif
    } else {
        ret = buffer_put_fd(rb, w->fd, -1, &eof);
        if (ret < 0) {
            uwsc_error(cl, UWSC_ERROR_IO, "read error");
            return;
        }
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
    
    if (cl->state == CLIENT_STATE_CONNECTING) {
        if (check_socket_state(cl) < 0)
            return;
    }

    if (cl->ssl) {
#ifdef SSL_SUPPORT
        static char err_buf[128];
        struct buffer *b = &cl->wb;

        if (unlikely(cl->state == CLIENT_STATE_SSL_HANDSHAKE)) {
            ret = ssl_negotiated(cl);
            if (ret <= 0)
                return;
        }

        ret = ssl_write(cl->ssl, buffer_data(b), buffer_length(b));
        if (ret == SSL_ERROR) {
            log_err("ssl_write(%d): %s\n", ssl_err_code,
                    ssl_strerror(ssl_err_code, err_buf, sizeof(err_buf)));
            uwsc_error(cl, UWSC_ERROR_IO, err_buf);
            return;
        }

        if (ret == SSL_PENDING)
            return;

        buffer_pull(b, NULL, ret);
#endif

    } else {
        ret = buffer_pull_to_fd(&cl->wb, w->fd, buffer_length(&cl->wb));
        if (ret < 0) {
            uwsc_error(cl, UWSC_ERROR_IO, "write error");
            return;
        }
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
        buffer_put_u16(wb, htobe16(len));
    } else {
        buffer_put_u8(wb, 0x80 | 127);
        buffer_put_u64(wb, htobe64(len));
    }

    buffer_put_data(wb, mk, 4);

    p = data;
    for (i = 0; i < len; i++)
        buffer_put_u8(wb, p[i] ^ mk[i % 4]);

    ev_io_start(cl->loop, &cl->iow);

    return 0;
}

int uwsc_send_ex(struct uwsc_client *cl, int op, int num, ...)
{
    struct buffer *wb = &cl->wb;
    const uint8_t *p;
    uint8_t mk[4];
    int len = 0;
    va_list ap;
    int i, j, k;

    get_nonce(mk, 4);

    buffer_put_u8(wb, 0x80 | op);

    va_start(ap, num);
    for (i = 0; i < num; i++) {
        len += va_arg(ap, int);
        va_arg(ap, int);
    }
    va_end(ap);

    if (len < 126) {
        buffer_put_u8(wb, 0x80 | len);
    } else if (len < 65536) {
        buffer_put_u8(wb, 0x80 | 126);
        buffer_put_u16(wb, htobe16(len));
    } else {
        buffer_put_u8(wb, 0x80 | 127);
        buffer_put_u64(wb, htobe64(len));
    }

    buffer_put_data(wb, mk, 4);

    k = 0;
    va_start(ap, num);
    for (i = 0; i < num; i++) {
        len = va_arg(ap, int);
        p = va_arg(ap, uint8_t *);

        for (j = 0; j < len; j++)
            buffer_put_u8(wb, p[j] ^ mk[(k + j) % 4]);
        k += len;
    }
    va_end(ap);

    ev_io_start(cl->loop, &cl->iow);

    return 0;
}

static inline void uwsc_ping(struct uwsc_client *cl)
{
    const char *msg = "libuwsc";
    cl->send(cl, msg, strlen(msg), UWSC_OP_PING);
}

static void uwsc_handshake(struct uwsc_client *cl, const char *host, int port,
    const char *path, const char *extra_header)
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

    if (extra_header && *extra_header)
        buffer_put_string(wb, extra_header);

    buffer_put_string(wb, "\r\n");

    ev_io_start(cl->loop, &cl->iow);
}

static void uwsc_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    struct uwsc_client *cl = container_of(w, struct uwsc_client, timer);
    ev_tstamp now = ev_now(loop);

    if (unlikely(cl->state == CLIENT_STATE_CONNECTING)) {
        if (now - cl->start_time > UWSC_MAX_CONNECT_TIME) {
            uwsc_error(cl, UWSC_ERROR_CONNECT, "Connect timeout");
            return;
        }
    }

    if (unlikely(cl->state < CLIENT_STATE_PARSE_MSG_HEAD))
        return;

    if (cl->ping_interval < 1)
        return;

    if (unlikely(cl->wait_pong)) {
        if (now - cl->last_ping < 5)
            return;

        cl->wait_pong = false;
        log_err("ping timeout %d\n", ++cl->ntimeout);
        if (cl->ntimeout > 2) {
            uwsc_error(cl, UWSC_ERROR_PING_TIMEOUT, "ping timeout");
            return;
        }
    } else {
        cl->ntimeout = 0;
    }

    if (now - cl->last_ping < cl->ping_interval)
        return;

    cl->ping(cl);
    cl->last_ping = now;
    cl->wait_pong = true;
}

struct uwsc_client *uwsc_new(struct ev_loop *loop, const char *url,
    int ping_interval, const char *extra_header)
{
    struct uwsc_client *cl;

    cl = malloc(sizeof(struct uwsc_client));
    if (!cl) {
        log_err("malloc failed: %s\n", strerror(errno));
        return NULL;
    }

    if (uwsc_init(cl, loop, url, ping_interval, extra_header) < 0) {
        free(cl);
        return NULL;
    }

    return cl;
}

#ifdef SSL_SUPPORT
#define SSL_CTX_CHECK                                       \
    do {                                                    \
        if (!ssl_ctx) {                                     \
            ssl_ctx = ssl_context_new(false);               \
            if (!ssl_ctx) {                                 \
                log_err("SSL context init fail\n");   \
                return -1;                                  \
            }                                               \
        }                                                   \
    } while (0)
#endif

int uwsc_init(struct uwsc_client *cl, struct ev_loop *loop, const char *url,
    int ping_interval, const char *extra_header)
{
    const char *path = "/";
    char host[256] = "";
    bool inprogress;
    int sock = -1;
    int port;
    bool ssl;
    int eai;

    memset(cl, 0, sizeof(struct uwsc_client));

    if (parse_url(url, host, sizeof(host), &port, &path, &ssl) < 0) {
        log_err("Invalid url\n");
        return -1;
    }

    sock = tcp_connect(host, port, SOCK_NONBLOCK | SOCK_CLOEXEC, &inprogress, &eai);
    if (sock < 0) {
        log_err("tcp_connect failed: %s\n", strerror(errno));
        return -1;
    } else if (sock == 0) {
        log_err("tcp_connect failed: %s\n", gai_strerror(eai));
        return -1;
    }

    if (!inprogress)
        cl->state = CLIENT_STATE_HANDSHAKE;

    cl->loop = loop ? loop : EV_DEFAULT;
    cl->sock = sock;
    cl->send = uwsc_send;
    cl->send_ex = uwsc_send_ex;
    cl->send_close = uwsc_send_close;
    cl->ping = uwsc_ping;
    cl->free = uwsc_free;
    cl->start_time = ev_now(cl->loop);
    cl->ping_interval = ping_interval;
    if (ssl) {
#ifdef SSL_SUPPORT
        SSL_CTX_CHECK;

        cl->ssl = ssl_session_new(ssl_ctx, sock);
        if (!cl->ssl) {
            log_err("SSL session init fail\n");
            return -1;
        }
#else
        log_err("SSL is not enabled at compile\n");
        uwsc_free(cl);
        return -1;
#endif
    }

    ev_io_init(&cl->iow, uwsc_io_write_cb, sock, EV_WRITE);

    ev_io_init(&cl->ior, uwsc_io_read_cb, sock, EV_READ);
    ev_io_start(cl->loop, &cl->ior);

    ev_timer_init(&cl->timer, uwsc_timer_cb, 0.0, 1.0);
    ev_timer_start(cl->loop, &cl->timer);

    uwsc_handshake(cl, host, port, path, extra_header);

    return 0;
}

#ifdef SSL_SUPPORT
int uwsc_load_ca_crt_file(const char *file)
{
    SSL_CTX_CHECK;

    return ssl_load_ca_crt_file(ssl_ctx, file);
}

int uwsc_load_crt_file(const char *file)
{
    SSL_CTX_CHECK;

    return ssl_load_crt_file(ssl_ctx, file);
}

int uwsc_load_key_file(const char *file)
{
    SSL_CTX_CHECK;

    return ssl_load_key_file(ssl_ctx, file);
}
#endif
