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

int parse_url(const char *url, char **host, int *port, const char **path, bool *ssl)
{
    char *p;
    const char *host_pos;
    int host_len = 0;

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

#if (UWSC_SSL_SUPPORT)
const struct ustream_ssl_ops *init_ustream_ssl()
{
    void *dlh;
    struct ustream_ssl_ops *ops;

    dlh = dlopen("libustream-ssl.so", RTLD_LAZY | RTLD_LOCAL);
    if (!dlh) {
        uwsc_log_err("Failed to load ustream-ssl library: %s", dlerror());
        return NULL;
    }

    ops = dlsym(dlh, "ustream_ssl_ops");
    if (!ops) {
        uwsc_log_err("Could not find required symbol 'ustream_ssl_ops' in ustream-ssl library");
        return NULL;
    }

    return ops;
}
#endif
