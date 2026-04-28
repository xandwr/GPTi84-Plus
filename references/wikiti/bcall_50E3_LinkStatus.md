# 83Plus:BCALLs:50E3

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:50E3

## Synopsis

**Unofficial Name:** LinkStatus

**BCALL Address:** 50E3

**Minimum OS Version:** 1.13

"Checks the status of port 0 or the link assist for activity, depending on
the model calculator."

## Inputs

None.

## Outputs

- NZ if calculator is busy

## Destroys

None.

## Comments

"This B_CALL was introduced in OS 1.13. Programs should check the OS version
before attempting to use this routine."

May be connected to related BCALLs 50E6 and 50E9.
