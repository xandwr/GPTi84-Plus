# 83Plus:BCALLs:4ED3

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4ED3

## Synopsis

**Unofficial Name:** SendRAMCmd

**BCALL Address:** 4ED3

"Sends almost any packet you want over the link port that contains data and
receives acknowledgement."

## Inputs

- A: command byte (15h for data packet, 06h/0A2h/0C9h for variable header, etc.)
- HL: address of variable data for data packets
- DE: size of packet data
- iy+1Bh: requires appropriate setup

## Outputs

None.

## Destroys

All registers.

## Comments

- Throws ERR:LINK if any problems are found.
- Exhibits unexpected behavior when sndRecState=08h, varClass=0Ah, and DE >= 037Dh.
- Recommended precaution: zero out sndRecState before calling to avoid that case.
