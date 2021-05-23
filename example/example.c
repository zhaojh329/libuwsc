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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "uwsc.h"

static void stdin_read_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
    struct uwsc_client *cl = w->data;
    char buf[128] = "";
    int n;

    n = read(w->fd, buf, sizeof(buf));
    if (n > 1) {
        buf[n - 1] = 0;

        if (buf[0] == 'q')
            cl->send_close(cl, UWSC_CLOSE_STATUS_NORMAL, "ByeBye");
        else
            cl->send(cl, buf, strlen(buf) + 1,  UWSC_OP_TEXT);
    }
}

static void uwsc_onopen(struct uwsc_client *cl)
{
    static struct ev_io stdin_watcher;

    log_info("onopen\n");

    stdin_watcher.data = cl;

    ev_io_init(&stdin_watcher, stdin_read_cb, STDIN_FILENO, EV_READ);
    ev_io_start(cl->loop, &stdin_watcher);

    /* Usage of send_ex */
    cl->send_ex(cl, UWSC_OP_TEXT,
        2, strlen("hello,"), "hello,", strlen("server"), "server");

    printf("Please input:\n");
}

static void uwsc_onmessage(struct uwsc_client *cl,
	void *data, size_t len, bool binary)
{
    printf("Recv:");

    if (binary) {
        size_t i;
        uint8_t *p = data;

        for (i = 0; i < len; i++) {
            printf("%02hhX ", p[i]);
            if (i % 16 == 0 && i > 0)
                puts("");
        }
        puts("");
    } else {
        printf("[%.*s]\n", (int)len, (char *)data);
    }

    printf("Please input:\n");
}

static void uwsc_onerror(struct uwsc_client *cl, int err, const char *msg)
{
    log_err("onerror:%d: %s\n", err, msg);
    ev_break(cl->loop, EVBREAK_ALL);
}

static void uwsc_onclose(struct uwsc_client *cl, int code, const char *reason)
{
    log_err("onclose:%d: %s\n", code, reason);
    ev_break(cl->loop, EVBREAK_ALL);
}

static void signal_cb(struct ev_loop *loop, ev_signal *w, int revents)
{
    if (w->signum == SIGINT) {
        ev_break(loop, EVBREAK_ALL);
        log_info("Normal quit\n");
    }
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "      -u url       # ws://localhost:8080/ws\n"
        "                     wss://localhost:8080/ws\n"
        "      -P n      	# Ping interval\n"
        , prog);
    exit(1);
}

int main(int argc, char **argv)
{
	const char *url = "ws://localhost:8080/ws";
    struct ev_loop *loop = EV_DEFAULT;
    struct ev_signal signal_watcher;
	int ping_interval = 10;	/* second */
    struct uwsc_client *cl;
	int opt;

    while ((opt = getopt(argc, argv, "u:P:")) != -1) {
        switch (opt) {
        case 'u':
            url = optarg;
            break;
		case 'P':
			ping_interval = atoi(optarg);
			break;
        default: /* '?' */
            usage(argv[0]);
        }
    }

	log_info("Libuwsc: %s\n", UWSC_VERSION_STRING);

    cl = uwsc_new(loop, url, ping_interval, NULL);
    if (!cl)
        return -1;

	log_info("Start connect...\n");

    cl->onopen = uwsc_onopen;
    cl->onmessage = uwsc_onmessage;
    cl->onerror = uwsc_onerror;
    cl->onclose = uwsc_onclose;

    ev_signal_init(&signal_watcher, signal_cb, SIGINT);
    ev_signal_start(loop, &signal_watcher);

    ev_run(loop, 0);

    free(cl);
    
    return 0;
}
