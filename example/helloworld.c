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

#include <uwsc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libubox/ulog.h>

struct uwsc_client *gcl;
struct uloop_fd fd;

void fd_handler(struct uloop_fd *u, unsigned int events)
{
    char buf[128] = "";
    int n;

    n = read(u->fd, buf, sizeof(buf));
    if (n > 1) {
        buf[n - 1] = 0;
        printf("You input:[%s]\n", buf);
        gcl->send(gcl, buf, strlen(buf) + 1, WEBSOCKET_OP_TEXT);
    }
}

static void uwsc_onopen(struct uwsc_client *cl)
{
    ULOG_INFO("onopen\n");

    fd.fd = STDIN_FILENO;
    fd.cb = fd_handler;
    uloop_fd_add(&fd, ULOOP_READ);
}

static void uwsc_onmessage(struct uwsc_client *cl, char *data, uint64_t len, enum websocket_op op)
{
    if (op == WEBSOCKET_OP_BINARY) {
        uint64_t i;
        for (i = 0; i < len; i++) {
            printf("%02hhX ", data[i]);
            if (i % 16 == 0 && i > 0)
                puts("");
        }
        puts("");
    } else if (op == WEBSOCKET_OP_TEXT) {
        printf("recv:[%s]\n", data);
    }
}

static void uwsc_onerror(struct uwsc_client *cl)
{
    ULOG_INFO("onerror:%d\n", cl->error);
}

static void uwsc_onclose(struct uwsc_client *cl)
{
    ULOG_INFO("onclose\n");
    uloop_end();
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "      -u url       # ws://localhost:8080/ws\n"
        "      -c file      # Load CA certificates from file\n"
        "      -n           # don't validate the server's certificate\n"
        "      -v           # verbose\n"
        , prog);
    exit(1);
}

int main(int argc, char **argv)
{
    int opt;
    const char *url = "ws://localhost:8080/ws";
    const char *crt_file = NULL;
    bool verify = true;
    bool verbose = false;

    while ((opt = getopt(argc, argv, "u:nc:v")) != -1) {
        switch (opt)
        {
        case 'u':
            url = optarg;
            break;
        case 'n':
            verify = false;
            break;
        case 'c':
            crt_file = optarg;
            break;
        case 'v':
            verbose = true;
            break;
        default: /* '?' */
            usage(argv[0]);
        }
    }

    if (!verbose)
        ulog_threshold(LOG_ERR);

    uloop_init();

    gcl = uwsc_new_ssl(url, crt_file, verify);
    if (!gcl) {
        uloop_done();
        return -1;
    }
   
    gcl->onopen = uwsc_onopen;
    gcl->onmessage = uwsc_onmessage;
    gcl->onerror = uwsc_onerror;
    gcl->onclose = uwsc_onclose;
    
    uloop_run();

    gcl->send(gcl, NULL, 0, WEBSOCKET_OP_CLOSE);
    gcl->free(gcl);

    uloop_done();
    
    return 0;
}
