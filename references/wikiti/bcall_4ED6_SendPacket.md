# 83Plus:BCALLs:4ED6

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4ED6

## Synopsis

**Unofficial Name:** SendPacket

**BCALL Address:** 4ED6

This routine enables transmission of packets through the link port with data
and awaits acknowledgement from the receiving device.

## Inputs

- (iMathPtr5): memory address containing the data for transmission
- header: packet start information (4 bytes comprising machine ID, command, and size word)
- iy+1Bh: must be configured appropriately

## Outputs

- B register: machine ID from the receiving calculator

## Destroys

All registers affected.

## Comments

The routine will generate an ERR:LINK error if any transmission difficulties
occur during operation.
