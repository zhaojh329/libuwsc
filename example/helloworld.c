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

static void uwsc_onopen(struct uwsc_client *cl)
{

}

static void uwsc_onmessage(struct uwsc_client *cl, char *data, uint64_t len)
{
	static int sent;
	char buf[1024] = "I'm libuwsc";

	printf("recv:[%.*s]\n", (int)len, data);

	if (!sent) {
		sent = 1;
		cl->send(cl, buf, strlen(buf) + 1, WEBSOCKET_OP_TEXT);
	}
}

static void uwsc_onerror(struct uwsc_client *cl)
{

}

static void uwsc_onclose(struct uwsc_client *cl)
{

}

int main(int argc, char **argv)
{
	struct uwsc_client *cl = NULL;

    uloop_init();

    cl = uwsc_new("ws://192.168.3.33:81/lua");
   
   	cl->onopen = uwsc_onopen;
    cl->onmessage = uwsc_onmessage;
    cl->onerror = uwsc_onerror;
    cl->onclose = uwsc_onclose;

    uloop_run();
    uloop_done();
    
	cl->free(cl);

    return 0;
}
