# 83Plus:BCALLs:4EEE

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4EEE

## Synopsis

**Unofficial Name:** GetDataPacket

**BCALL Address:** 4EEE

"Receives a data packet over the link port and sends either acknowledge or
bad checksum packets."

## Inputs

- DE: address to receive data
- (arcInfo): page of address to receive data (if in Flash ROM)
- BC: expected size of data packet
- iy+1Bh set up accordingly

## Outputs

None

## Destroys

All registers

## Comments

"Will throw ERR:LINK if any problems are found." Operates similarly to
GetSmallPacket but receives the first 4 bytes of the packet.

"If the checksum is valid, an acknowledgement packet is sent. Otherwise, a
bad checksum packet is sent and ERR:LINK is thrown."
