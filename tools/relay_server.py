"""Lightweight TCP relay for the calc<->desktop chat bridge.

Listens on a TCP port, reads length-prefixed frames (4-byte big-endian
length + payload), prints each frame to stdout. v0 is calc-to-desktop
only; the reverse direction is intentionally not wired up yet.
"""

import argparse
import datetime as dt
import socketserver
import struct
import sys


def _read_exact(sock, n):
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            return None
        buf.extend(chunk)
    return bytes(buf)


class FramedHandler(socketserver.BaseRequestHandler):
    def handle(self):
        peer = "%s:%d" % self.client_address
        print("[%s] connected: %s" % (_now(), peer), flush=True)
        try:
            while True:
                hdr = _read_exact(self.request, 4)
                if hdr is None:
                    break
                (length,) = struct.unpack(">I", hdr)
                if length > 1 << 20:
                    print("[%s] %s: oversize frame %d, closing" % (_now(), peer, length), flush=True)
                    return
                body = _read_exact(self.request, length)
                if body is None:
                    print("[%s] %s: short read mid-frame" % (_now(), peer), flush=True)
                    return
                try:
                    text = body.decode("ascii")
                    pretty = repr(text)
                except UnicodeDecodeError:
                    pretty = body.hex()
                print("[%s] %s len=%d %s" % (_now(), peer, length, pretty), flush=True)
        finally:
            print("[%s] disconnected: %s" % (_now(), peer), flush=True)


def _now():
    return dt.datetime.now().strftime("%H:%M:%S")


class ReusingServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=9999)
    args = ap.parse_args(argv)
    with ReusingServer((args.host, args.port), FramedHandler) as srv:
        print("relay: listening on %s:%d" % (args.host, args.port), flush=True)
        try:
            srv.serve_forever()
        except KeyboardInterrupt:
            print("\nrelay: shutting down", flush=True)


if __name__ == "__main__":
    sys.exit(main())
