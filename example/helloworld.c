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

struct uwsc_client *cl;
struct uloop_fd fd;

void fd_handler(struct uloop_fd *u, unsigned int events)
{
    char buf[128] = "";
    int n;

    n = read(u->fd, buf, sizeof(buf));
    if (n > 1) {
        buf[n - 1] = 0;
        printf("You input:[%s]\n", buf);
        cl->send(cl, buf, strlen(buf) + 1, WEBSOCKET_OP_TEXT);
    }
}

static void uwsc_onopen(struct uwsc_client *cl)
{
    uwsc_log_debug("onopen");

    fd.fd = STDIN_FILENO;
    fd.cb = fd_handler;
    uloop_fd_add(&fd, ULOOP_READ);
}

static void uwsc_onmessage(struct uwsc_client *cl, char *data, uint64_t len, enum websocket_op op)
{
	printf("recv:[%.*s]\n", (int)len, data);
}

static void uwsc_onerror(struct uwsc_client *cl)
{
    printf("onerror:%d\n", cl->error);
}

static void uwsc_onclose(struct uwsc_client *cl)
{
    printf("onclose\n");

    uloop_done();
}

int main(int argc, char **argv)
{
    uloop_init();

    cl = uwsc_new("ws://127.0.0.1:81/lua");
    //cl = uwsc_new("wss://127.0.0.1:81/lua");
   
   	cl->onopen = uwsc_onopen;
    cl->onmessage = uwsc_onmessage;
    cl->onerror = uwsc_onerror;
    cl->onclose = uwsc_onclose;
    
    uloop_run();
    uloop_done();
    
	cl->free(cl);

    return 0;
}
