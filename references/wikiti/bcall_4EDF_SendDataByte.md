# 83Plus:BCALLs:4EDF

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4EDF

## Synopsis

**Unofficial Name:** SendDataByte

**BCALL Address:** 4EDF

"Sends a byte over the linkport and adds its value to the checksum."

## Inputs

- C: byte to send
- (header+4): old checksum value
- iy+1Bh: requires proper setup

## Outputs

- (header+4): updated checksum value

## Destroys

- All registers

## Comments

- "Will throw ERR:LINK if any problems are found."
- The checksum location at (header+4) "should be set to zero before the first call."
