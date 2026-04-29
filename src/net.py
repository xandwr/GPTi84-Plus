"""Pico W wifi + framed-TCP/WSS client for the chat bridge.

Loads SSID/PSK and SERVER_* from `secrets`. Frames are 4-byte big-endian
length followed by N payload bytes. Two transports are supported: raw
TCP (LAN dev / nimble direct) and WebSocket-over-TLS (Cloudflare Tunnel
front-ended by `relay.xandwr.com`). Selection is by `secrets.SERVER_WSS`
(falsy / missing -> raw TCP, truthy -> WSS).

The bridge layer treats both transports identically: send_framed() and
FrameReader use the same sendall/recv calls. WSClient buffers writes so
that send_framed's two sendalls (length prefix, then body) coalesce into
exactly one WebSocket binary message per frame, and surfaces recv() over
the byte stream of accumulated message payloads.
"""

import binascii
import network
import os
import socket
import ssl
import struct
import time

import secrets


def connect_wifi(timeout_s=15):
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        print("wifi: connecting to", secrets.WIFI_SSID)
        wlan.connect(secrets.WIFI_SSID, secrets.WIFI_PASSWORD)
        deadline = time.ticks_add(time.ticks_ms(), timeout_s * 1000)
        while not wlan.isconnected():
            if time.ticks_diff(deadline, time.ticks_ms()) <= 0:
                raise OSError("wifi connect timed out")
            time.sleep_ms(200)
    print("wifi: connected, ifconfig=", wlan.ifconfig())
    _sync_clock()
    return wlan


def _sync_clock():
    """Best-effort NTP sync. The Pico has no battery-backed RTC, so on
    every boot it starts at epoch (2020-something on this firmware) --
    that breaks TLS cert-validity checks because production certs look
    like they were issued in the future. Failure is non-fatal: raw TCP
    paths don't need wallclock, and WSS connects will surface a clearer
    error from the cert validator."""
    try:
        import ntptime
    except ImportError:
        print("clock: ntptime not available, skipping")
        return
    for attempt in range(3):
        try:
            ntptime.settime()
            print("clock: NTP sync ok, now=", time.gmtime())
            return
        except OSError as e:
            print("clock: NTP attempt", attempt + 1, "failed:", e)
            time.sleep_ms(500)
    print("clock: NTP sync gave up; TLS may reject cert validity")


def open_socket(host=None, port=None):
    """Return an object exposing sendall(bytes), recv(n), setblocking(flag),
    close(). For raw TCP that's a stdlib socket; for WSS that's a WSClient
    that speaks one-WS-binary-message-per-framed-packet to the bridge."""
    host = host if host is not None else secrets.SERVER_HOST
    port = port if port is not None else secrets.SERVER_PORT
    if getattr(secrets, "SERVER_WSS", False):
        path = getattr(secrets, "WS_PATH", "/")
        client_id = getattr(secrets, "CF_ACCESS_CLIENT_ID", None)
        client_secret = getattr(secrets, "CF_ACCESS_CLIENT_SECRET", None)
        ws = WSClient.connect(host, port, path,
                              client_id=client_id,
                              client_secret=client_secret)
        print("net: WSS connected to", host, port, path)
        return ws
    addr = socket.getaddrinfo(host, port)[0][-1]
    s = socket.socket()
    s.connect(addr)
    print("net: connected to", host, port)
    return s


def send_framed(sock, payload):
    if not isinstance(payload, (bytes, bytearray, memoryview)):
        raise TypeError("payload must be bytes-like")
    sock.sendall(struct.pack(">I", len(payload)))
    sock.sendall(bytes(payload))


def _recv_n_blocking(sock, n):
    """Read exactly n bytes from a blocking socket. None on clean close."""
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            return None
        buf.extend(chunk)
    return bytes(buf)


class FrameReader:
    """Stateful non-blocking framed-frame reader.

    Drives a single TCP socket in non-blocking mode. `poll()` returns a
    completed payload (bytes) if one is ready, or None if not enough data
    has arrived yet. Frame format: 4-byte big-endian length + payload.

    Use this from the bridge supervisor's idle path so we can interleave
    with DBUS work without blocking.
    """
    def __init__(self, sock):
        self.sock = sock
        sock.setblocking(False)
        self._hdr = bytearray()
        self._body = bytearray()
        self._needed = None     # None until header parsed

    def poll(self):
        try:
            if self._needed is None:
                while len(self._hdr) < 4:
                    chunk = self.sock.recv(4 - len(self._hdr))
                    if chunk is None:
                        return None         # MicroPython: no data available
                    if chunk == b"":
                        raise OSError("peer closed")
                    print("FrameReader: hdr chunk len=", len(chunk),
                          "bytes=", bytes(chunk))
                    self._hdr.extend(chunk)
                (self._needed,) = struct.unpack(">I", bytes(self._hdr))
                print("FrameReader: parsed length=", self._needed)
                if self._needed > 1 << 20:
                    raise OSError("oversize frame")
            while len(self._body) < self._needed:
                chunk = self.sock.recv(self._needed - len(self._body))
                if chunk is None:
                    return None
                if chunk == b"":
                    raise OSError("peer closed")
                print("FrameReader: body chunk len=", len(chunk))
                self._body.extend(chunk)
            payload = bytes(self._body)
            self._hdr = bytearray()
            self._body = bytearray()
            self._needed = None
            return payload
        except OSError as e:
            # MicroPython's non-blocking recv may either return None,
            # raise OSError(EAGAIN), or behave like CPython depending on
            # build. Treat EAGAIN as "no frame yet". Other OSError
            # (peer closed, oversize, real socket error) propagates.
            errno = e.args[0] if e.args else 0
            if errno in (11, 35, 110):  # EAGAIN linux/darwin, ETIMEDOUT
                return None
            print("FrameReader: OSError errno=", errno, "args=", e.args)
            raise


# RFC 6455 magic GUID. Server appends this to the client's Sec-WebSocket-Key
# and SHA-1s the result; we don't validate the response value because TLS
# already authenticates the server identity, and Cloudflare Access has
# already authorised the connection by the time we got here.
_WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


class _WSClosed(OSError):
    pass


class WSClient:
    """Minimal WebSocket-over-TLS client for the bridge transport.

    Speaks "one binary WS message = one framed packet" with the server-
    side ws_bridge: each frame the bridge layer builds via send_framed
    (4-byte length + body) is coalesced into a single WS binary message,
    and each inbound message is exposed to the bridge as a stream of
    recv()-able bytes. send_framed and FrameReader work without changes.

    Concessions to MicroPython: no kwargs on encode/decode, no os.urandom
    on every build (we fall back to time-based mask seeding if missing),
    no ssl.SSLContext in some firmware revs (use ssl.wrap_socket).
    """

    def __init__(self, sock):
        self.sock = sock
        self._send_buf = bytearray()
        self._recv_buf = bytearray()
        # Incremental WS frame parser state; used in non-blocking pumps
        # so a partially-arrived frame can be resumed across recv calls.
        self._frame_state = "hdr2"      # hdr2 -> ext_len -> body
        self._frame_b1 = 0
        self._frame_b2 = 0
        self._frame_n = 0               # body length once known
        self._frame_partial = None      # bytearray accumulator for current chunk
        self._frame_body = bytearray()  # body bytes gathered so far
        self._closed = False
        self._blocking = True
        # Bytes already pulled off the socket during the HTTP upgrade
        # handshake but not yet consumed by the WS parser. Drained first
        # by _try_read before touching the socket.
        self._prepend = bytearray()

    @classmethod
    def connect(cls, host, port, path,
                client_id=None, client_secret=None,
                handshake_timeout_s=30):
        addr = socket.getaddrinfo(host, port)[0][-1]
        raw = socket.socket()
        raw.settimeout(handshake_timeout_s)
        raw.connect(addr)
        print("WS: TCP connected, starting TLS handshake")
        sock = cls._tls_wrap(raw, host)
        print("WS: TLS handshake complete, starting WS upgrade")
        leftover = cls._do_handshake(sock, host, path, client_id, client_secret)
        print("WS: handshake complete")
        sock.setblocking(True)
        client = cls(sock)
        # Bytes the server sent past \r\n\r\n are the start of WS frames;
        # the parser must see them before any new socket reads.
        if leftover:
            client._prepend.extend(leftover)
        return client

    @staticmethod
    def _tls_wrap(raw, host):
        """Wrap a TCP socket in TLS with full peer-cert verification
        against our bundled root CAs. SNI is required so Cloudflare's
        edge picks the right cert, hostname check confirms we're talking
        to the host we asked for."""
        # Lazy import so non-WSS paths don't pay the cost.
        import ca_bundle
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        # MicroPython 1.28 ssl.load_verify_locations only accepts a
        # single DER blob via cadata per call; loop the bundle.
        for der in ca_bundle.CA_DER_LIST:
            ctx.load_verify_locations(cadata=der)
        ctx.verify_mode = ssl.CERT_REQUIRED
        # check_hostname is on by default for PROTOCOL_TLS_CLIENT but be
        # explicit -- without it a valid cert for any other domain in
        # bundled root would be accepted.
        try:
            ctx.check_hostname = True
        except AttributeError:
            # Older builds expose check via verify_mode only; cert
            # validation still requires the hostname to match SAN.
            pass
        return ctx.wrap_socket(raw, server_hostname=host)

    @staticmethod
    def _make_key():
        # 16 random bytes, base64 encoded. binascii.b2a_base64 appends \n
        # which we strip.
        try:
            raw = os.urandom(16)
        except (AttributeError, OSError):
            t = time.ticks_us()
            raw = bytes((t >> (i % 32)) & 0xFF for i in range(16))
        b64 = binascii.b2a_base64(raw)
        # Strip trailing \n. b2a_base64 returns bytes.
        if b64.endswith(b"\n"):
            b64 = b64[:-1]
        return b64

    @staticmethod
    def _do_handshake(sock, host, path, client_id, client_secret):
        key = WSClient._make_key()
        host_hdr = host
        lines = [
            "GET " + path + " HTTP/1.1",
            "Host: " + host_hdr,
            "Upgrade: websocket",
            "Connection: Upgrade",
            "Sec-WebSocket-Key: " + key.decode(),
            "Sec-WebSocket-Version: 13",
        ]
        if client_id:
            lines.append("CF-Access-Client-Id: " + client_id)
        if client_secret:
            lines.append("CF-Access-Client-Secret: " + client_secret)
        req = ("\r\n".join(lines) + "\r\n\r\n").encode("ascii")
        sock.write(req)
        # Read until \r\n\r\n. Pico's ssl.read() can block waiting for
        # more bytes than are actually available, so read in tiny chunks
        # until we see the header terminator.
        buf = bytearray()
        deadline = time.ticks_add(time.ticks_ms(), 15000)
        while b"\r\n\r\n" not in buf:
            if time.ticks_diff(deadline, time.ticks_ms()) <= 0:
                raise OSError("WS handshake timeout")
            chunk = sock.read(1)
            if chunk is None:
                # Non-blocking sentinel -- shouldn't happen here but yield.
                time.sleep_ms(20)
                continue
            if not chunk:
                raise OSError("WS handshake: peer closed")
            buf.extend(chunk)
            if len(buf) > 4096:
                raise OSError("WS handshake: response too large")
        end = buf.index(b"\r\n\r\n")
        head = bytes(buf[:end])
        leftover = bytes(buf[end + 4:])
        # Parse status line.
        status_line = head.split(b"\r\n", 1)[0]
        parts = status_line.split(b" ", 2)
        if len(parts) < 2 or parts[1] != b"101":
            # Surface server response head for debugging (e.g. 403 from
            # Access if the service token headers are missing/wrong).
            raise OSError("WS handshake bad status: " + repr(head[:200]))
        # We don't validate Sec-WebSocket-Accept; TLS + Access already
        # authenticate the server. Return any post-header bytes so the
        # caller can feed them to the WS frame parser.
        if leftover:
            print("WS: handshake had", len(leftover), "leftover bytes")
        return leftover

    @staticmethod
    def _make_mask():
        try:
            return os.urandom(4)
        except (AttributeError, OSError):
            t = time.ticks_us()
            return bytes(((t >> (i * 8)) & 0xFF) for i in range(4))

    def _send_message(self, payload, opcode=0x2):
        """Send one WS frame with FIN=1 and the given opcode (default
        binary). Client-to-server frames must be masked per RFC 6455."""
        n = len(payload)
        hdr = bytearray()
        hdr.append(0x80 | (opcode & 0x0F))   # FIN | opcode
        if n < 126:
            hdr.append(0x80 | n)             # MASK | len
        elif n < (1 << 16):
            hdr.append(0x80 | 126)
            hdr.append((n >> 8) & 0xFF)
            hdr.append(n & 0xFF)
        else:
            hdr.append(0x80 | 127)
            for sh in (56, 48, 40, 32, 24, 16, 8, 0):
                hdr.append((n >> sh) & 0xFF)
        mask = self._make_mask()
        hdr.extend(mask)
        masked = bytearray(n)
        for i in range(n):
            masked[i] = payload[i] ^ mask[i & 3]
        self.sock.write(bytes(hdr))
        if n:
            self.sock.write(bytes(masked))

    def _read_exact(self, n):
        """Blocking exact read. Caller is expected to have blocking mode
        set; in non-blocking mode use _try_read_exact instead."""
        buf = bytearray()
        while len(buf) < n:
            chunk = self.sock.read(n - len(buf))
            if not chunk:
                raise _WSClosed("WS peer closed")
            buf.extend(chunk)
        return bytes(buf)

    def _try_read(self, n):
        """One non-blocking read of up to n bytes. Returns bytes (possibly
        empty-on-EAGAIN sentinel via None), or raises _WSClosed on clean
        peer close."""
        if self._prepend:
            chunk = bytes(self._prepend[:n])
            self._prepend = self._prepend[n:]
            return chunk
        try:
            chunk = self.sock.read(n)
        except OSError as e:
            errno = e.args[0] if e.args else 0
            if errno in (11, 35, 110):  # EAGAIN / ETIMEDOUT
                return None
            raise
        if chunk is None:
            return None
        if chunk == b"":
            raise _WSClosed("WS peer closed")
        return chunk

    def _try_fill(self, n):
        """Top up self._frame_partial to n bytes using non-blocking reads.
        Returns True when complete, False if more bytes are needed."""
        if self._frame_partial is None:
            self._frame_partial = bytearray()
        while len(self._frame_partial) < n:
            chunk = self._try_read(n - len(self._frame_partial))
            if chunk is None:
                return False
            self._frame_partial.extend(chunk)
        return True

    def _consume_partial(self):
        out = bytes(self._frame_partial)
        self._frame_partial = None
        return out

    def _pump_one_step(self):
        """Advance the WS frame parser by reading whatever's available.
        Returns one of:
          'progress'  -- consumed bytes, possibly completed a data frame
                         (in which case its payload is now in _recv_buf)
          'no-data'   -- read got EAGAIN; caller should yield
          'closed'    -- peer closed cleanly
        """
        try:
            if self._frame_state == "hdr2":
                if not self._try_fill(2):
                    return "no-data"
                hdr = self._consume_partial()
                self._frame_b1 = hdr[0]
                self._frame_b2 = hdr[1]
                masked = (self._frame_b2 & 0x80) != 0
                if masked:
                    raise OSError("WS server frame masked (protocol error)")
                n = self._frame_b2 & 0x7F
                if n == 126:
                    self._frame_state = "ext16"
                elif n == 127:
                    self._frame_state = "ext64"
                else:
                    self._frame_n = n
                    self._frame_body = bytearray()
                    self._frame_state = "body"
                return "progress"
            if self._frame_state == "ext16":
                if not self._try_fill(2):
                    return "no-data"
                ext = self._consume_partial()
                (self._frame_n,) = struct.unpack(">H", ext)
                self._frame_body = bytearray()
                self._frame_state = "body"
                return "progress"
            if self._frame_state == "ext64":
                if not self._try_fill(8):
                    return "no-data"
                ext = self._consume_partial()
                (self._frame_n,) = struct.unpack(">Q", ext)
                self._frame_body = bytearray()
                self._frame_state = "body"
                return "progress"
            if self._frame_state == "body":
                if self._frame_n > 0:
                    chunk = self._try_read(self._frame_n - len(self._frame_body))
                    if chunk is None:
                        return "no-data"
                    self._frame_body.extend(chunk)
                    if len(self._frame_body) < self._frame_n:
                        return "progress"
                fin = (self._frame_b1 & 0x80) != 0
                opcode = self._frame_b1 & 0x0F
                payload = bytes(self._frame_body)
                # Reset for next frame.
                self._frame_state = "hdr2"
                self._frame_b1 = 0
                self._frame_b2 = 0
                self._frame_n = 0
                self._frame_body = bytearray()
                if not fin:
                    raise OSError("WS fragmentation not supported")
                if opcode == 0x9:       # ping -> pong
                    self._send_message(payload, opcode=0xA)
                elif opcode == 0xA:     # pong: ignore
                    pass
                elif opcode == 0x8:     # close
                    try:
                        self._send_message(b"", opcode=0x8)
                    except OSError:
                        pass
                    self._closed = True
                    return "closed"
                elif opcode in (0x1, 0x2):  # text or binary
                    self._recv_buf.extend(payload)
                else:
                    print("WS: unknown opcode", opcode, "skipped")
                return "progress"
        except _WSClosed:
            self._closed = True
            return "closed"
        return "progress"

    def _pump_until_data(self):
        """Drive the parser, blocking, until at least one data byte lands
        in _recv_buf or the connection closes. Used in blocking mode."""
        while not self._recv_buf:
            r = self._pump_one_step()
            if r == "closed":
                return False
            if r == "no-data":
                # Blocking sock returning no-data means we should poll
                # rather than spin; a short sleep keeps us friendly.
                time.sleep_ms(10)
        return True

    # Stdlib socket-shaped API used by send_framed and FrameReader.

    def sendall(self, data):
        """Buffer until a full framed packet (4-byte len + body) is ready,
        then ship it as one WS binary message. send_framed calls sendall
        twice (prefix, body); coalesce so the bridge sees exactly one
        message per frame."""
        if self._closed:
            raise OSError("WS closed")
        self._send_buf.extend(data)
        # Drain as many complete frames as possible.
        while len(self._send_buf) >= 4:
            (length,) = struct.unpack(">I", bytes(self._send_buf[:4]))
            total = 4 + length
            if len(self._send_buf) < total:
                break
            frame = bytes(self._send_buf[:total])
            self._send_buf = self._send_buf[total:]
            self._send_message(frame, opcode=0x2)

    def recv(self, n):
        """Return up to n bytes. In blocking mode, waits for data; in
        non-blocking mode, returns None when no data is buffered (matches
        what FrameReader expects -- it treats None as 'no frame yet').
        Empty bytes (b'') signals clean close."""
        if self._recv_buf:
            chunk = bytes(self._recv_buf[:n])
            self._recv_buf = self._recv_buf[n:]
            return chunk
        if self._closed:
            return b""
        if self._blocking:
            if not self._pump_until_data():
                return b""
        else:
            # Drive the parser until either a data frame lands or we'd
            # block; return None to signal "try again later".
            while not self._recv_buf:
                r = self._pump_one_step()
                if r == "closed":
                    return b""
                if r == "no-data":
                    return None
        if not self._recv_buf:
            return None if not self._blocking else b""
        chunk = bytes(self._recv_buf[:n])
        self._recv_buf = self._recv_buf[n:]
        return chunk

    def setblocking(self, flag):
        self._blocking = bool(flag)
        self.sock.setblocking(flag)

    def close(self):
        if self._closed:
            return
        self._closed = True
        try:
            self._send_message(b"", opcode=0x8)
        except OSError:
            pass
        try:
            self.sock.close()
        except OSError:
            pass
