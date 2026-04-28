"""Validate the .8Xp extractor against the real artifacts checked into the repo,
and against an in-memory file built by hand.

Both FLAPPY.8xp and the SEX debug program are real DBUS_STATE-proven payloads,
so anything that breaks parsing here is also breaking the e2e push path.
"""

from pathlib import Path

import pytest

from extract_8xp import parse_8xp, read_8xp


REPO = Path(__file__).resolve().parent.parent


def test_flappy_payload_matches_sidecar():
    """programs/flappy_bird/FLAPPY.payload was extracted from FLAPPY.8xp by
    earlier work; the extractor must now produce the same bytes."""
    name, type_id, locked, payload = read_8xp(REPO / "programs" / "flappy_bird" / "FLAPPY.8xp")
    expected = (REPO / "programs" / "flappy_bird" / "FLAPPY.payload").read_bytes()
    assert payload == expected
    assert name == "FLAPPY"
    assert type_id in (0x05, 0x06)
    # FLAPPY.payload is 1754 bytes per DBUS_STATE.md
    assert len(payload) == 1754


def test_sex_debug_program_extracts():
    name, type_id, locked, payload = read_8xp(REPO / "programs" / "debug" / "SEX.8xp")
    assert name == "SEX"
    # Per DBUS_STATE.md: SEX has body b'\x0b\x00\xde*SEXY)LOL*' (13 bytes)
    assert payload == b"\x0b\x00\xde*SEXY)LOL*"


def test_rejects_short_buffer():
    with pytest.raises(ValueError):
        parse_8xp(b"")


def test_rejects_bad_signature():
    fake = b"BADSIGN!" + b"\x00" * 200
    with pytest.raises(ValueError):
        parse_8xp(fake)
