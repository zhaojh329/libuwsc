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

static void uwsc_process_headers(struct uwsc_client *cl)
{
    enum {
        HTTP_HDR_CONNECTION,
        HTTP_HDR_UPGRADE,
        HTTP_HDR_SEC_WEBSOCKET_ACCEPT,
        __HTTP_HDR_MAX,
    };
    static const struct blobmsg_policy hdr_policy[__HTTP_HDR_MAX] = {
#define hdr(_name) { .name = _name, .type = BLOBMSG_TYPE_STRING }
        [HTTP_HDR_CONNECTION] = hdr("connection"),
        [HTTP_HDR_UPGRADE] = hdr("upgrade"),
        [HTTP_HDR_SEC_WEBSOCKET_ACCEPT] = hdr("sec-websocket-accept")
#undef hdr
    };
    struct blob_attr *tb[__HTTP_HDR_MAX];
    struct blob_attr *cur;

    blobmsg_parse(hdr_policy, __HTTP_HDR_MAX, tb, blob_data(cl->meta.head), blob_len(cl->meta.head));

    cur = tb[HTTP_HDR_CONNECTION];
    if (!cur || !strstr(blobmsg_data(cur), "upgrade")) {
        printf("Invalid connection\n");
    }

    cur = tb[HTTP_HDR_UPGRADE];
    if (!cur || !strstr(blobmsg_data(cur), "websocket")) {
        printf("Invalid upgrade\n");
    }

    cur = tb[HTTP_HDR_SEC_WEBSOCKET_ACCEPT];
    if (!cur) {
        printf("Invalid sec-websocket-accept\n");
    }
}

static void uwsc_headers_complete(struct uwsc_client *cl)
{
    cl->state = CLIENT_STATE_RECV_DATA;
    uwsc_process_headers(cl);
}

static void uwsc_parse_http_line(struct uwsc_client *cl, char *data)
{
    char *name;
    char *sep;

    if (cl->state == CLIENT_STATE_REQUEST_DONE) {
        char *code;

        if (!strlen(data))
            return;

        /* HTTP/1.1 */
        strsep(&data, " ");

        code = strsep(&data, " ");
        if (!code)
            goto error;

        cl->status_code = strtoul(code, &sep, 10);
        if (sep && *sep)
            goto error;

        cl->state = CLIENT_STATE_RECV_HEADERS;
        return;
    }

    if (!*data) {
        uwsc_headers_complete(cl);
        return;
    }

    sep = strchr(data, ':');
    if (!sep)
        return;

    *(sep++) = 0;

    for (name = data; *name; name++)
        *name = tolower(*name);

    name = data;
    while (isspace(*sep))
        sep++;

    blobmsg_add_string(&cl->meta, name, sep);
    return;

error:
    cl->status_code = 400;
    cl->eof = true;
    //uclient_notify_eof(uh);
}

static int parse_frame(struct uwsc_frame *frame, char *data, int len)
{
    frame->data = data;
    frame->fin = (data[0] & 0x80) ? 1 : 0;
    frame->opcode = data[0] & 0x0F;

    if (data[1] & 0x80) {
        uwsc_log_err("Masked error");
        return -1;
    }

    frame->payload_offset = 2;
    frame->payload_len = data[1] & 0x7F;

    switch (frame->payload_len) {
    case 126:
        frame->payload_len = ntohs(*(uint16_t *)&data[2]);
        frame->payload_offset += 2;
        break;
    case 127:
        frame->payload_len = (((uint64_t)ntohl(*(uint32_t *)&data[2])) << 32) + ntohl(*(uint32_t *)&data[6]);
        frame->payload_offset += 8;
        break;
    default:
        break;
    }

    return 0;
}

static void client_ustream_read_cb(struct ustream *s, int bytes)
{
    struct uwsc_client *cl = container_of(s, struct uwsc_client, sfd.stream);
    unsigned int seq = cl->seq;
    char *data;
    int len;

    if (cl->state < CLIENT_STATE_REQUEST_DONE || cl->state == CLIENT_STATE_ERROR)
        return;

    data = ustream_get_read_buf(s, &len);
    if (!data || !len)
        return;

    if (cl->state < CLIENT_STATE_RECV_DATA) {
        char *sep, *next;
        int cur_len;

        do {
            sep = strchr(data, '\n');
            if (!sep)
                break;

            next = sep + 1;
            if (sep > data && sep[-1] == '\r')
                sep--;

            /* Check for multi-line HTTP headers */
            if (sep > data) {
                if (!*next)
                    return;

                if (isspace(*next) && *next != '\r' && *next != '\n') {
                    sep[0] = ' ';
                    if (sep + 1 < next)
                        sep[1] = ' ';
                    continue;
                }
            }

            *sep = 0;
            cur_len = next - data;
            uwsc_parse_http_line(cl, data);
            if (seq != cl->seq)
                return;

            ustream_consume(cl->us, cur_len);
            len -= cur_len;

            if (cl->eof)
                return;

            data = ustream_get_read_buf(cl->us, &len);
        } while (data && cl->state < CLIENT_STATE_RECV_DATA);
    }

    if (cl->eof)
        return;

    if (cl->state == CLIENT_STATE_RECV_DATA && data) {
        if (!parse_frame(&cl->frame, data, len))
            cl->onmessage(cl, cl->frame.data + cl->frame.payload_offset, cl->frame.payload_len);
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
    //char buf[] = {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58};
    char buf[1024] = "";
    char *p = buf;
    struct timeval tv;
    unsigned char mask[4];
    unsigned int mask_int;
    int i;

    gettimeofday(&tv, NULL);
    srand(tv.tv_usec * tv.tv_sec);
    mask_int = rand();
    memcpy(mask, &mask_int, 4);

    *p++ = 0x81;

    if (len < 126) {
        *p++ = 0x80 | len;
        memcpy(p, mask, 4);

        memcpy(p + 4, data, len);
        p += 4;

        for (i = 0; i < len; i++) {
            p[i] ^= mask[i % 4];
        }
    }

    ustream_write(cl->us, buf, 1 + 1 + 4 + len, false);

    return 0;
}

struct uwsc_client *uwsc_new(const char *url)
{
    struct uwsc_client *cl = NULL;
    char *host = NULL;
    const char *path = "/";
    int port = 80;
    int sock = -1;
    char key_nonce[16];
    char websocket_key[256] = "";
    int i;

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

    blob_buf_init(&cl->meta, 0);

    srand(time(NULL));
    for(i = 0; i< 16; i++) {
        key_nonce[i] = rand() & 0xff;
    }

    b64_encode(key_nonce, 16, websocket_key, sizeof(websocket_key));

    ustream_printf(cl->us, "GET %s HTTP/1.1\r\n", path);
    ustream_printf(cl->us, "Upgrade: websocket\r\n");
    ustream_printf(cl->us, "Connection: Upgrade\r\n");
    ustream_printf(cl->us, "Host: %s:%d\r\n", host, port);
    ustream_printf(cl->us, "Sec-WebSocket-Key: %s\r\n", websocket_key);
    ustream_printf(cl->us, "Sec-WebSocket-Version: 13\r\n");
    ustream_printf(cl->us, "\r\n");

    cl->state = CLIENT_STATE_REQUEST_DONE;

    free(host);

    return cl;

err:
    if (host)
        free(host);

    if (cl)
        cl->free(cl);

    return NULL;    
}    
