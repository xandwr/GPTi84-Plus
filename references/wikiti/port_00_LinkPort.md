# 83Plus:Ports:00 - Link Port

Source: https://wikiti.brandonw.net/index.php?title=83Plus:Ports:00

## Overview

Port 00h controls the serial link port on TI-83+, 83+ SE, 84+, and 84+ SE
calculators. This is distinct from the USB port on 84+/84+ SE.

## Read Operations

- **Bits 0-1:** link port line states (tip and ring). High = 1, low = 0. Both lines typically read high when idle.
- **Bit 2 (83+ only):** shows if link receive assist is active.
- **Bit 3 (83+ only):** set when link assist receives a complete byte; reset by reading port 5.
- **Bits 4-5:** reflect which lines *your* calculator is holding low (not influenced by the remote device). Mirror the most recent write.
- **Bit 6 (83+ only):** set when link assist is actively receiving data.

## Write Operations

- **Bits 0-1:** setting a bit pulls the line low. Clearing it releases the line (allowing it to go high if the remote device isn't holding it).
- **Bit 2 (83+ only):** enable link receive assist; poll port 0 for bit 3, then read port 5 for the received byte.

## Key Considerations

"TI-OS checks for silent transfers in the background" even during input
routines, causing automatic acknowledgment signals that can complicate
synchronization code.

## Code Examples

Sending (line control):

```
ld a,0     ; both lines high
out (0),a
ld a,2     ; tip high, ring low
out (0),a
```

Receiving (line checking):

```
in a,(0)
bit 0,a    ; check tip
jr z,tip_low
```
