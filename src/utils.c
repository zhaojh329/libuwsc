/*
 * MIT License
 *
 * Copyright (c) 2019 Jianhui Zhao <jianhuizhao329@gmail.com>
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

/* reference from https://tools.ietf.org/html/rfc4648#section-4 */
int b64_encode(const void *src, size_t srclen, void *dest, size_t destsize)
{
    char *Base64 =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint8_t *input = src;
    char *output = dest;

    while (srclen > 0) {
        int i0 = input[0] >> 2;
		int skip = 1;

        if (destsize < 5)
            return -1;

        *output++ = Base64[i0];

        if (srclen > 1) {
            int i1 = ((input[0] & 0x3) << 4) + (input[1] >> 4);
            *output++ = Base64[i1];
            if (srclen > 2) {
                int i2 = ((input[1] & 0xF) << 2) + (input[2] >> 6);
                int i3 = input[2] & 0x3F;
                *output++ = Base64[i2];
                *output++ = Base64[i3];
                skip = 3;
            } else {
                int i2 = ((input[1] & 0xF) << 2);
                *output++ = Base64[i2];
                *output++ = '=';
                skip = 2;
            }
        } else {
            int i1 = (input[0] & 0x3) << 4;
            *output++ = Base64[i1];
            *output++ = '=';
            *output++ = '=';
        }

        input += skip;
        srclen -= skip;
        destsize -= 4;
    }

    *output++ = 0;
    return output - (char *)dest - 1;
}
