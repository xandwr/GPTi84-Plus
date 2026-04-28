# 83Plus:Hooks:9BD4 - USB Activity Hook

Source: https://wikiti.brandonw.net/index.php?title=83Plus:Hooks:9BD4

## Synopsis

- **Hook Name:** USB Activity Hook
- **Hook Pointer Block:** 9BD4
- **Enable BCALL:** 528A (`_EnableUSBHook`)
- **Disable BCALL:** 528D (`_DisableUSBHook`)
- **Call BCALL:** None known
- **Active Flag:** bit 0 of (iy + 3Ah)
- **OS Requirement:** introduced in OS 2.30. Apps should check OS version.

## When it fires

"Called from the interrupt routine when USB activity is detected." Fires
whenever any of bits 4-0 of port 55h are low.

## Contract

**Input:**
- B = always 2Ch
- C = complement of bits 4-0 of port 55h

**Output:**
- Set B=0 to abort linking (similar to silent-link hook semantics)

## Credits

Discovered and analyzed by 84plusfreak (Sernin van de Krol).

## Notes for our use case

DBUS-domain locked, so USB hook is not on our path. Saved for completeness
in case the DBUS lock ever lifts.
