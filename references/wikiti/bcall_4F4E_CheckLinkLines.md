# 83Plus:BCALLs:4F4E

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4F4E

## Synopsis

**Unofficial Name:** CheckLinkLines

**BCALL Address:** 4F4E

"Checks the link port data lines to see if they match the value in A."

## Inputs

- A: value to check against (mask out bits 2-7)
- set 0,(iy+3Eh) if link assist is active

## Outputs

- Z flag set if link port matches the value in A

## Destroys

All registers.

## Comments

"A pretty dumb entry point," which may verify a stable link connection. The
OS link activity hook utilizes this routine.

Inactive when bit 0 at (iy+3Eh) is set.
