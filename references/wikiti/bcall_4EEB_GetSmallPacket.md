# 83Plus:BCALLs:4EEB

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4EEB

## Synopsis

**Unofficial Name:** GetSmallPacket

**BCALL Address:** 4EEB

This function receives up to 14 bytes from a data packet transmitted through
the link port and responds with either an acknowledgement or bad checksum
notification.

## Inputs

- (header+2): anticipated packet data size
- iy+1Bh: must be configured appropriately

## Outputs

- Received bytes are stored in ioData location

## Destroys

- All registers

## Comments

The routine will generate an ERR:LINK exception if errors occur during
transmission. "It DOES NOT receive the first 4 bytes of the data packet
(machine ID, command, size word)."

When checksum validation succeeds, an acknowledgement packet is transmitted.
If validation fails, a bad checksum packet is sent along with ERR:LINK being
thrown.

BCALL 4F8A functions identically except it directs received bytes to
ioData-1 instead.
