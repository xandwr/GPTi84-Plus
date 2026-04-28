# 83Plus:BCALLs:4EDC

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4EDC

## Synopsis

**Unofficial Name:** Send4BytePacket

**BCALL Address:** 4EDC

This routine enables transmission of a 4-byte packet and waits for
acknowledgement over the link port.

## Inputs

- **H register:** command to send (for example, 09h)
- **iy+1Bh:** must be configured appropriately

## Outputs

None specified.

## Destroys

All registers.

## Comments

The routine will generate an ERR:LINK error if complications occur, including
receipt of a non-acknowledgement response.

The pair of bytes transmitted after the machine ID and command can be
customized by modifying the header at RAM location +2 before invoking this
BCALL entry point.

Categories: 83Plus:BCALLs:By Name:Link, 83Plus:BCALLs:By Name, 83Plus:BCALLs:By Address.
