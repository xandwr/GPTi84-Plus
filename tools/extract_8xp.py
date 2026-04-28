"""Pull the variable payload bytes out of a .8Xp (single-program) file.

The .8Xp layout per DBUS_STATE.md:

  [8-byte sig "**TI83F*"]
  [3-byte sub 1A 0A 00]
  [42-byte comment]
  [2-byte data section length le16]
  [var entries...]
  [2-byte file checksum le16]

Each var entry:

  [2-byte entry-header-length=0x000D]
  [2-byte var data size le16]
  [1-byte type ID]
  [8-byte name]
  [1-byte version=0]
  [1-byte flags=0]
  [2-byte var data size le16]   # repeated
  [var data]

The "var data" returned here is exactly what put_prog_83p() wants as its
payload: [2-byte token-stream length le16][token bytes].
"""

from pathlib import Path


HEADER_LEN = 8 + 3 + 42 + 2  # sig + sub + comment + data-section-length
ENTRY_FIXED_LEN = 17         # 2 + 2 + 1 + 8 + 1 + 1 + 2


def parse_8xp(buf):
    """Return (name, type_id, locked, payload_bytes) for the first var entry.

    Raises ValueError if the file isn't a recognizable .8Xp.
    """
    if len(buf) < HEADER_LEN + ENTRY_FIXED_LEN + 2:
        raise ValueError("file too short to be a .8Xp")
    if buf[:8] != b"**TI83F*":
        raise ValueError("missing **TI83F* signature")

    p = HEADER_LEN
    entry_hdr_len = buf[p] | (buf[p + 1] << 8)
    if entry_hdr_len != 0x000D:
        raise ValueError("unexpected entry header length {:#x}".format(entry_hdr_len))
    var_size_a = buf[p + 2] | (buf[p + 3] << 8)
    type_id = buf[p + 4]
    name = bytes(buf[p + 5:p + 13])
    var_size_b = buf[p + 15] | (buf[p + 16] << 8)
    if var_size_a != var_size_b:
        raise ValueError("var size fields disagree: {} vs {}".format(var_size_a, var_size_b))

    payload = bytes(buf[p + ENTRY_FIXED_LEN:p + ENTRY_FIXED_LEN + var_size_a])
    if len(payload) != var_size_a:
        raise ValueError("payload truncated: want {}, got {}".format(var_size_a, len(payload)))

    locked = (type_id == 0x06)
    return name.rstrip(b"\x00").decode("ascii"), type_id, locked, payload


def read_8xp(path):
    return parse_8xp(Path(path).read_bytes())


if __name__ == "__main__":
    import sys
    name, type_id, locked, payload = read_8xp(sys.argv[1])
    sys.stderr.write(
        "name={} type=0x{:02X} locked={} payload_len={}\n".format(
            name, type_id, locked, len(payload)
        )
    )
    sys.stdout.buffer.write(payload)
