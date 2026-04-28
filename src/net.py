"""Pico W wifi + framed-TCP client for the chat bridge.

Loads SSID/PSK and SERVER_HOST/SERVER_PORT from `secrets`. Frames are
4-byte big-endian length followed by N payload bytes.
"""

import network
import socket
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
    return wlan


def open_socket(host=None, port=None):
    host = host if host is not None else secrets.SERVER_HOST
    port = port if port is not None else secrets.SERVER_PORT
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
