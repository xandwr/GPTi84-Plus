from packet import (
    checksum, packet_bytes,
    MACHINE_ID, ACK, CTS, RTS, DATA,
    pc_id_for,
)


def test_checksum_is_low_16_bits_of_byte_sum():
    assert checksum(b"") == 0
    assert checksum(b"\x01\x02\x03") == 6
    # wrap above 16 bits
    assert checksum(b"\xFF" * 300) == (255 * 300) & 0xFFFF


def test_packet_bytes_no_data_omits_checksum():
    pkt = packet_bytes(ACK, b"")
    assert pkt == bytes([MACHINE_ID, ACK, 0x00, 0x00])


def test_packet_bytes_with_data_appends_le_checksum():
    body = b"\x01\x02\x03"
    pkt = packet_bytes(DATA, body)
    cs = sum(body) & 0xFFFF
    assert pkt == bytes([MACHINE_ID, DATA, len(body), 0x00]) + body + bytes(
        [cs & 0xFF, (cs >> 8) & 0xFF]
    )


def test_pc_id_for_known_pairs():
    # From packet.html: calc-side ID maps to PC-side reply ID.
    assert pc_id_for(0x73) == 0x23
    assert pc_id_for(0x82) == 0x02
    assert pc_id_for(0x83) == 0x03
    # unknown calc ID falls back to the 83+/84+ default
    assert pc_id_for(0xFF) == 0x23


def test_linkguide_rts_real_a_example():
    """RTS for real "A" with future DATA size 9: linkguide example body and checksum.

    Body: [09 00 00 41 00 00 00 00 00 00 00 00 00], checksum 0x4A.
    """
    body = bytes([0x09, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00])
    assert checksum(body) == 0x4A
    pkt = packet_bytes(RTS, body)
    assert pkt[0] == MACHINE_ID
    assert pkt[1] == RTS
    assert pkt[2] == 13 and pkt[3] == 0
    assert pkt[4:4 + 13] == body
    assert pkt[-2] == 0x4A and pkt[-1] == 0x00
