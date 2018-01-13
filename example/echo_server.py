#!/usr/bin/env python3

import asyncio
import websockets
import ssl
import getopt
import sys

async def hello(websocket, path):
    while True:
        data = await websocket.recv()
        print('<', data)
        await websocket.send(data)
        print('>', data)

port = 8080
use_ssl = False

opts, args = getopt.getopt(sys.argv[1:], "p:s")
for op, value in opts:
    if op == '-p':
        port = int(value)
    elif op == '-s':
        use_ssl = True

context = None
if use_ssl:
    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile = 'server-cert.pem', keyfile = 'server-key.pem')

print('Listen on:', port)
print('Use ssl:', use_ssl)

start_server = websockets.serve(hello, '0.0.0.0', port, ssl = context)

loop = asyncio.get_event_loop()
loop.run_until_complete(start_server)
loop.run_forever()
