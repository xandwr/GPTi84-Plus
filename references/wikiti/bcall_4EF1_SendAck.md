# 83Plus:BCALLs:4EF1

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4EF1

## Synopsis

**Unofficial Name:** SendAck

**BCALL Address:** 4EF1

Transmits an acknowledgement packet through the link port.

## Inputs

- iy+1Bh must be configured appropriately

## Outputs

- None

## Destroys

- All registers

## Comments

Generates ERR:LINK on communication problems. The final two bytes of the
transmitted packet can be customized by modifying data at header+2 before
invoking this BCALL.
