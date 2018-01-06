/*
 * The Action handler is a simple libuhttpd handler that processes requests
 * by invoking registered C functions. The action handler is ideal for
 * situations when you want to generate a simple response using C code. 
 *
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

static void uwsc_onopen(struct uwsc_client *cl)
{

}

static void uwsc_onmessage(struct uwsc_client *cl)
{

}

static void uwsc_onerror(struct uwsc_client *cl)
{

}

static void uwsc_onclose(struct uwsc_client *cl)
{

}

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

static void client_ustream_read_cb(struct ustream *s, int bytes)
{
    int len;
    char *str;

    str = ustream_get_read_buf(s, &len);
    if (!str || !len)
        return;

    printf("read:%s\n", str);
}

static void client_ustream_write_cb(struct ustream *s, int bytes)
{
}

static void client_notify_state(struct ustream *s)
{
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

    cl->onopen = uwsc_onopen;
    cl->onmessage = uwsc_onmessage;
    cl->onerror = uwsc_onerror;
    cl->onclose = uwsc_onclose;
    cl->free = uwsc_free;

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

    free(host);

    return cl;

err:
    if (host)
        free(host);

    if (cl)
        cl->free(cl);

    return NULL;    
}    
