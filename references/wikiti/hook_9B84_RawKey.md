# 83Plus:Hooks:9B84 - Raw Key Hook

Source: https://wikiti.brandonw.net/index.php?title=83Plus:Hooks:9B84

## Synopsis

- **Hook Name:** Raw Key (officially defined in ti83plus.inc)
- **Hook Pointer Block:** 9B84
- **Enable BCALL:** 4F66 (`_SetGetKeyHook`)
- **Disable BCALL:** 4F6F (`_ClrRawKeyHook`)
- **Call BCALL:** 4F5D (`_call_rawkey_hook`)
- **Active Flag:** bit 5 of (iy + 34h)

## When it fires

Any time a key is accepted by GetKey. Special handling for kOff only when
bit 7 of (iy+28h) is set.

## Contract

**Inputs:**
- A = keycode
- (keyExtend at RAM 8446) = extended keycode

**Outputs:**
- Modify A and/or keyExtend to alter the "pressed" key.
- Return Z OR A=0 to ignore the keypress.
- Return A non-zero with NZ to accept the (possibly modified) keypress.

## Notes for our use case

The Raw Key hook fires AFTER the OS has accepted a key. The GetCSC hook
(9B88) fires DURING the keypad scan, which is earlier. For checking NVRAM
between keypresses while `_GetKey` is blocking, GetCSC is the better fit.

Raw Key is more useful for swapping/intercepting specific key behavior
(e.g. remapping the [MODE] key inside CHAT).

## Naming caveat

The page warns: "The official name may be misleading; it functions as a
GetKey hook rather than a raw key handler, though renaming remains
discouraged to avoid confusion with other hooks." Take the name with a
grain of salt.
