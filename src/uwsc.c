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

#include "uwsc.h"
#include "log.h"
#include "utils.h"
#include <libubox/usock.h>

static int uwsc_parse_url(const char *url, char **host, int *port, const char **path)
{
    char *p;
    const char *host_pos;
    int host_len = 0;

    if (strncmp(url, "ws://", 5))
        return -1;
    
    url += 5;
    host_pos = url;

    p = strchr(url, ':');
    if (p) {
        host_len = p - url;
        url = p + 1;
        *port = atoi(url);
    }

    p = strchr(url, '/');
    if (p) {
        *path = p;
        if (host_len == 0)
            host_len = p - host_pos;
    }

    if (host_len == 0)
        host_len = strlen(host_pos);

    *host = strndup(host_pos, host_len);

    return 0;
}

static void uwsc_free(struct uwsc_client *cl)
{
    ustream_free(&cl->sfd.stream);
    shutdown(cl->sfd.fd.fd, SHUT_RDWR);
    close(cl->sfd.fd.fd);
    free(cl);
}

static int parse_frame(struct uwsc_client *cl, char *data)
{
    struct uwsc_frame *frame = &cl->frame;
    
    frame->fin = (data[0] & 0x80) ? 1 : 0;
    frame->opcode = data[0] & 0x0F;

    if (data[1] & 0x80) {
        uwsc_log_err("Masked error");
        return -1;
    }

    frame->payload_len = data[1] & 0x7F;
    frame->payload = data + 2;

    switch (frame->payload_len) {
    case 126:
        frame->payload_len = ntohs(*(uint16_t *)&data[2]);
        frame->payload += 2;
        break;
    case 127:
        frame->payload_len = (((uint64_t)ntohl(*(uint32_t *)&data[2])) << 32) + ntohl(*(uint32_t *)&data[6]);
        frame->payload += 8;
        break;
    default:
        break;
    }

    uwsc_log_debug("FIN:%d", frame->fin);
    uwsc_log_debug("OP Code:%d", frame->opcode);
    uwsc_log_debug("Payload len:%d", (int)frame->payload_len);
    uwsc_log_debug("Payload:[%.*s]", (int)frame->payload_len, frame->payload);

    if (frame->fin) {
        if (cl->onmessage)
            cl->onmessage(cl, frame->payload, frame->payload_len, frame->opcode);
    }

    return 0;
}

static int parse_header(struct uwsc_client *cl, char *data)
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

        uwsc_log_debug("%s:%s", k, v);

        if (!strcasecmp(k, "Upgrade") && !strcasecmp(v, "websocket"))
            has_upgrade = true;

        if (!strcasecmp(k, "Connection") && !strcasecmp(v, "upgrade"))
            has_connection = true;

        if (!strcasecmp(k, "Sec-WebSocket-Accept"))
            has_sec_webSocket_accept = true;

        /* TODO: verify the value of Sec-WebSocket-Accept */
    }

    if (!has_upgrade || !has_connection || !has_sec_webSocket_accept) {
        cl->state = CLIENT_STATE_ERROR;
        uwsc_log_err("Invalid header");
        return -1;
    }
    
    return 0;
}

static void uwsc_ping_cb(struct uloop_timeout *timeout)
{
    struct uwsc_client *cl = container_of(timeout, struct uwsc_client, timeout);

    if (cl->state > CLIENT_STATE_MESSAGE)
        return;

    cl->ping(cl);
    uloop_timeout_set(&cl->timeout, UWSC_PING_INTERVAL * 1000);
}

static void client_ustream_read_cb(struct ustream *s, int bytes)
{
    struct uwsc_client *cl = container_of(s, struct uwsc_client, sfd.stream);
    char *data;
    int len;

    if (cl->state > CLIENT_STATE_MESSAGE)
        return;

    data = ustream_get_read_buf(s, &len);
    if (!data || !len)
        return;

    if (cl->state == CLIENT_STATE_HANDSHAKE) {
        char *p, *version, *status_code, *summary;

        p = strstr(data, "\r\n\r\n");
        if (!p)
            return;
        
        p[2] = 0;

        version = strtok(data, " ");
        status_code = strtok(NULL, " ");
        summary = strtok(NULL, "\r\n");

        if (!version || strcmp(version, "HTTP/1.1")) {
            uwsc_log_err("Invalid version");
            cl->state = CLIENT_STATE_ERROR;
            return;
        }

        if (!status_code || atoi(status_code) != 101) {
            uwsc_log_err("Invalid status code");
            cl->state = CLIENT_STATE_ERROR;
            return;
        }

        if (!summary) {
            uwsc_log_err("Invalid summary");
            cl->state = CLIENT_STATE_ERROR;
            return;
        }

        if (parse_header(cl, data))
            return;

        ustream_consume(cl->us, p + 4 - data);

        if (cl->onopen)
            cl->onopen(cl);

        cl->timeout.cb = uwsc_ping_cb;
        uloop_timeout_set(&cl->timeout, UWSC_PING_INTERVAL * 1000);
        
        cl->state = CLIENT_STATE_MESSAGE;
    } else if (cl->state == CLIENT_STATE_MESSAGE) {
        parse_frame(cl, data);
    }
}

static void client_ustream_write_cb(struct ustream *s, int bytes)
{
}

static void client_notify_state(struct ustream *s)
{
}

static int uwsc_send(struct uwsc_client *cl, char *data, int len, enum websocket_op op)
{
    char buf[1024] = "";
    char *p = buf;
    struct timeval tv;
    unsigned char mask[4];
    unsigned int mask_int;
    int i;
    int frame_size = 0;

    gettimeofday(&tv, NULL);
    srand(tv.tv_usec * tv.tv_sec);
    mask_int = rand();
    memcpy(mask, &mask_int, 4);

    *p++ = 0x80 | op;

    if (len < 126) {
        frame_size = 6 + len;
        *p++ = 0x80 | len;
    
        memcpy(p, mask, 4);

        memcpy(p + 4, data, len);
        p += 4;

        for (i = 0; i < len; i++) {
            p[i] ^= mask[i % 4];
        }
    }

    ustream_write(cl->us, buf, frame_size, false);

    return 0;
}

static inline void uwsc_ping(struct uwsc_client *cl)
{
    cl->send(cl, NULL, 0, WEBSOCKET_OP_PING);
}

struct uwsc_client *uwsc_new(const char *url)
{
    struct uwsc_client *cl = NULL;
    char *host = NULL;
    const char *path = "/";
    int port = 80;
    int sock = -1;
    uint8_t nonce[16];
    char websocket_key[256] = "";

    if (uwsc_parse_url(url, &host, &port, &path) < 0) {
        uwsc_log_err("Invalid url");
        return NULL;
    }

    sock = usock(USOCK_TCP, host, usock_port(port));
    if (sock < 0) {
        uwsc_log_err("usock");
        goto err;
    }

    cl = calloc(1, sizeof(struct uwsc_client));
    if (!cl) {
        uwsc_log_err("calloc");
        goto err;
    }

    cl->us = &cl->sfd.stream;

    cl->us->notify_read = client_ustream_read_cb;
    cl->us->notify_write = client_ustream_write_cb;
    cl->us->notify_state = client_notify_state;

    cl->us->string_data = true;
    ustream_fd_init(&cl->sfd, sock);

    cl->free = uwsc_free;
    cl->send = uwsc_send;
    cl->ping = uwsc_ping;

    get_nonce(nonce, sizeof(nonce));

    b64_encode(nonce, sizeof(nonce), websocket_key, sizeof(websocket_key));

    ustream_printf(cl->us, "GET %s HTTP/1.1\r\n", path);
    ustream_printf(cl->us, "Upgrade: websocket\r\n");
    ustream_printf(cl->us, "Connection: Upgrade\r\n");
    ustream_printf(cl->us, "Host: %s:%d\r\n", host, port);
    ustream_printf(cl->us, "Sec-WebSocket-Key: %s\r\n", websocket_key);
    ustream_printf(cl->us, "Sec-WebSocket-Version: 13\r\n");
    ustream_printf(cl->us, "\r\n");

    cl->state = CLIENT_STATE_HANDSHAKE;

    free(host);

    return cl;

err:
    if (host)
        free(host);

    if (cl)
        cl->free(cl);

    return NULL;    
}    
