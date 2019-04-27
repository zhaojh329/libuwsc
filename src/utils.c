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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "log.h"
#include "utils.h"

int get_nonce(uint8_t *dest, int len)
{
    FILE *fp;
    size_t n;

    fp = fopen("/dev/urandom", "r");
    if (fp) {
        n = fread(dest, len, 1, fp);
        fclose(fp);
        return n;
    }

    return -1;
}

int parse_url(const char *url, char *host, int host_len,
    int *port, const char **path, bool *ssl)
{
    char *p;
    const char *host_pos;
    int hl = 0;

    if (!url)
        return -1;

    if (!strncmp(url, "ws://", 5)) {
        *ssl = false;
        url += 5;
        *port = 80;
    } else if (!strncmp(url, "wss://", 6)) {
        *ssl = true;
        url += 6;
        *port = 443;
    } else {
        return -1;
    }
    
    host_pos = url;

    p = strchr(url, ':');
    if (p) {
        hl = p - url;
        url = p + 1;
        *port = atoi(url);
    }

    p = strchr(url, '/');
    if (p) {
        *path = p;
        if (hl == 0)
            hl = p - host_pos;
    }

    if (hl == 0)
        hl = strlen(host_pos);

    if (hl > host_len - 1)
        hl = host_len - 1;

    memcpy(host, host_pos, hl);

    return 0;
}

static const char *port2str(int port)
{
    static char buffer[sizeof("65535\0")];

    if (port < 0 || port > 65535)
        return NULL;

    snprintf(buffer, sizeof(buffer), "%u", port);

    return buffer;
}

int tcp_connect(const char *host, int port, int flags, bool *inprogress, int *eai)
{
    int ret;
    int sock = -1;
    int addr_len;
    struct sockaddr *addr = NULL;
    struct addrinfo *result, *rp;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_ADDRCONFIG
    };

    *inprogress = false;

    ret = getaddrinfo(host, port2str(port), &hints, &result);
    if (ret) {
        if (ret == EAI_SYSTEM)
            return -1;
        *eai =  ret;
        return 0;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            addr = rp->ai_addr;
            addr_len = rp->ai_addrlen;
            break;
        }
    }

    if (!addr)
        goto free_addrinfo;

    sock = socket(AF_INET, SOCK_STREAM | flags, 0);
    if (sock < 0)
        goto free_addrinfo;

    if (connect(sock, addr, addr_len) < 0) {
        if (errno != EINPROGRESS) {
            close(sock);
            sock = -1;
        } else {
            *inprogress = true;
        }
    }

free_addrinfo:
    freeaddrinfo(result);
    return sock;
}
