# 83Plus:BCALLs:4EF4

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4EF4

## Synopsis

**Unofficial Name:** Get4Bytes

**BCALL Address:** 4EF4

Receives 4 bytes over the link port.

## Inputs

None.

## Outputs

The 4 bytes received are stored to the header location in RAM (8674h).

## Destroys

All registers.

## Comments

- Generates ERR:LINK if problems occur during transmission.
- Specifically receives the beginning of a valid packet, which must include a valid machine ID.
- Useful to call before other link routines that don't independently receive the first 4 bytes.
- Validates the packet structure before accepting the data.
