"""WebSocket front-end that proxies framed-TCP to the local relay.

Run on the same host as relay_server.py. Each inbound WS connection opens
a paired TCP connection to the relay; binary WS messages are forwarded as
already-framed bytes (4-byte big-endian length + body, the relay's native
format) and reverse traffic is reassembled into discrete WS messages so a
single TCP frame maps 1:1 to a single WS message.

Sits behind cloudflared, which proxies https://relay.xandwr.com/ws to
http://localhost:8080 (this server) and everything else to the console on
8081. Cloudflare Access enforces the service-token gate at the edge for
the Pico path and the GitHub-SSO gate for the human path, so this process
trusts that anyone who reached it is authorised.

Optional ingest: every WS frame in either direction (plus connect/close
events) is POSTed fire-and-forget to --ingest-url so the console can show
them live. Ingest failure never blocks or kills the wire.
"""

import argparse
import asyncio
import binascii
import json
import struct
import sys
import time
import urllib.error
import urllib.request
import uuid

import websockets


PREVIEW_BYTES = 64


def _preview(data):
    snippet = bytes(data[:PREVIEW_BYTES])
    hex_str = binascii.hexlify(snippet).decode("ascii")
    ascii_str = "".join(chr(b) if 0x20 <= b < 0x7F else "." for b in snippet)
    if len(data) > PREVIEW_BYTES:
        hex_str += "..."
        ascii_str += "..."
    return hex_str, ascii_str


class Ingest:
    def __init__(self, url):
        self.url = url
        self._loop = asyncio.get_event_loop() if url else None

    def _post_sync(self, payload):
        try:
            req = urllib.request.Request(
                self.url,
                data=json.dumps(payload).encode("utf-8"),
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            urllib.request.urlopen(req, timeout=0.25).read()
        except (urllib.error.URLError, OSError, TimeoutError):
            pass

    def emit(self, *, conn_id, direction, length=0, data=None, note=None):
        if not self.url:
            return
        if data is not None:
            hex_str, ascii_str = _preview(data)
        else:
            hex_str, ascii_str = "", note or ""
        payload = {
            "ts_ms": int(time.time() * 1000),
            "direction": direction,
            "length": length,
            "preview_hex": hex_str,
            "preview_ascii": ascii_str,
            "conn_id": conn_id,
            "note": note,
        }
        loop = asyncio.get_event_loop()
        loop.run_in_executor(None, self._post_sync, payload)


async def _read_frame(reader):
    hdr = await reader.readexactly(4)
    (length,) = struct.unpack(">I", hdr)
    body = await reader.readexactly(length)
    return hdr + body, body


async def _ws_to_tcp(ws, writer, ingest, conn_id):
    async for msg in ws:
        if isinstance(msg, str):
            continue
        writer.write(msg)
        await writer.drain()
        body = msg[4:] if len(msg) >= 4 else msg
        ingest.emit(conn_id=conn_id, direction="c2s", length=len(body), data=body)


async def _tcp_to_ws(reader, ws, ingest, conn_id):
    while True:
        try:
            framed, body = await _read_frame(reader)
        except asyncio.IncompleteReadError:
            return
        await ws.send(framed)
        ingest.emit(conn_id=conn_id, direction="s2c", length=len(body), data=body)


async def handle(ws, relay_host, relay_port, ingest):
    conn_id = uuid.uuid4().hex[:8]
    peer = ws.remote_address
    print("ws: connected %s path=%s id=%s" % (peer, ws.request.path, conn_id), flush=True)
    ingest.emit(conn_id=conn_id, direction="open",
                note="ws open from %s:%d path=%s" % (peer[0], peer[1], ws.request.path))
    try:
        reader, writer = await asyncio.open_connection(relay_host, relay_port)
    except OSError as e:
        print("ws: relay dial failed (%s); closing %s" % (e, peer), flush=True)
        ingest.emit(conn_id=conn_id, direction="info", note="relay dial failed: %s" % e)
        await ws.close(code=1011, reason="relay unavailable")
        return
    try:
        await asyncio.gather(
            _ws_to_tcp(ws, writer, ingest, conn_id),
            _tcp_to_ws(reader, ws, ingest, conn_id),
            return_exceptions=False,
        )
    except websockets.ConnectionClosed:
        pass
    finally:
        writer.close()
        try:
            await writer.wait_closed()
        except OSError:
            pass
        print("ws: disconnected %s" % (peer,), flush=True)
        ingest.emit(conn_id=conn_id, direction="close", note="ws closed")


async def main_async(args):
    ingest = Ingest(args.ingest_url)

    async def handler(ws):
        await handle(ws, args.relay_host, args.relay_port, ingest)

    print("ws_bridge: listening on %s:%d -> relay %s:%d ingest=%s" % (
        args.host, args.port, args.relay_host, args.relay_port,
        args.ingest_url or "off"), flush=True)
    async with websockets.serve(handler, args.host, args.port):
        await asyncio.Future()


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("--relay-host", default="127.0.0.1")
    ap.add_argument("--relay-port", type=int, default=9999)
    ap.add_argument("--ingest-url", default="",
                    help="POST event JSON here per frame (e.g. http://127.0.0.1:8081/api/ingest)")
    args = ap.parse_args(argv)
    try:
        asyncio.run(main_async(args))
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    sys.exit(main())
