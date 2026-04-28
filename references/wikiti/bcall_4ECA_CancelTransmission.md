# 83Plus:BCALLs:4ECA

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4ECA

## Synopsis

**Unofficial Name:** CancelTransmission

**BCALL Address:** 4ECA

"Sends skip/exit packet and receives acknowledgement packet."

## Inputs

- A: skip/exit code (1 for exit, 2 for skip, 3 for out-of-memory, etc.)
- DE: number of bytes to send (theoretically, for skip/exit codes with data attached)
- iy+1Bh set up accordingly
- 867C: data to send (the value in A is placed at 867C)

## Outputs

None.

## Destroys

All.

## Comments

"Will throw ERR:LINK if any problems are found."

Differs from SendSkipExitPacket in that it can transmit skip/exit packets
with attached data. However, no known skip/exit codes actually utilize this
capability. To replicate SendSkipExitPacket, simply pass 1 in DE.
