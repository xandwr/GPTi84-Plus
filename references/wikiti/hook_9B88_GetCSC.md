# 83Plus:Hooks:9B88 - GetCSC Hook

Source: https://wikiti.brandonw.net/index.php?title=83Plus:Hooks:9B88

## Synopsis

- **Official Name:** GetCSC (also called GetKey hook, naming debated)
- **Hook Pointer Block Address:** 9B88
- **Hook Enable BCALL:** 4F7B (`_SetGetcscHook`)
- **Hook Disable BCALL:** 4F7E (`_ClrGetKeyHook`)
- **Hook Call BCALL:** 4F78 (`_getkeyhook`)
- **Hook Active Flag:** bit 0 of (iy + 34h)

## Overview

Called when the OS scans for keypad activity during the GetKey routine.
Lower level than the Raw Key hook : operates on raw scan codes rather than
processed key values.

## Using the Hook

The hook receives a value in A that selects the call site:

**A = 1Ah (before keypad scan):**
- Return with Z flag set and A = keycode to force a key press.
- Return with NZ flag set and A = 1Ah to proceed with normal scanning.

**A = 1Bh (after keypad scan):**
- B contains the scancode found (or 0 if no key pressed).
- Return with NZ flag set and A = scancode to be returned.

## Example use

WikiTI's example demonstrates swapping 2nd and Alpha by checking which key
was pressed and returning the alternate scancode.

## Comments

Deals with raw scan codes rather than fully processed keypresses (which
depend on 2nd / Alpha modifier state).

## Notes for our use case

This is the OS-level keypress hook : NOT the silent-link hook. The silent-link
hook is `_SetSilentLinkHook` at $50CEh (see TI 83+ include) and lives at a
different RAM pointer block.

**However**, this hook may turn out to be load-bearing for the chat use case
in a way the dead-ends memory didn't consider:

The dead-ends memory found that `bcall(_GetKey)` from inside running asm
unwinds the asm program when silent-link activity arrives. But what if we
install a GetCSC hook BEFORE calling `_GetKey`, so that *every* keypad scan
gives our hook a chance to run? The hook fires while `_GetKey` is blocking,
which means:

- It runs from the same OS context where the silent-link service runs.
- We can `_ChkFindSym` for CHATIN inside the hook (carefully).
- Returning A=1Ah / NZ keeps `_GetKey` blocking : no OS unwind.

This is at minimum worth investigating before committing to building
calc-master REQ from `_SendPacket` primitives. Caveats:

- Hooks must be installed by a *resident* program (per project_os_integration_hooks.md). Our CHAT.z80 is launched from homescreen and exits to homescreen, not resident. Either install/uninstall around the chat session, or accept that the hook persists.
- Doing meaningful work (NVRAM lookup + screen redraw) inside a hook is risky : hooks are expected to return quickly. The safer pattern is "set a flag in our own RAM, return 1Ah/NZ, let the main loop check the flag after `_GetKey` returns from a real keypress."
- That last bullet means we still need a real keypress to wake `_GetKey`. So it doesn't actually solve the chat-without-keystrokes case. It only solves "render the inbound message the next time the user presses any key," which is a UX downgrade from real-time but better than "exit and re-enter."

So: this is a candidate path 1.5, between option 3 ("concede, two-step UX")
and option 1 ("calc-master REQ from low-level primitives") in the dead-ends
memory's ranking. Possibly worth a short experiment, but not a clear win.
