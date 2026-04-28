# 83Plus:OS:Hooks - calling-convention overview

Source: https://wikiti.brandonw.net/index.php?title=83Plus:OS:Hooks

## What hooks are

Concealed OS feature originally designed for official TI flash applications.
Lets a program intercept OS execution at specific points to modify values or
extend functionality. Works in both flash and RAM-based programs.

## Installation

1. Place the hook code at a stable memory location.
   - Flash apps: natural protection.
   - RAM-based hooks: place away from temporary buffers like AppBackUpScreen.
2. Hook code MUST begin with `.db $83`. The OS validates this byte before
   jumping to the hook : prevents jumping to corrupted hooks.
3. Load the hook's address into HL and its page number into A (page 1 for RAM).
4. Invoke the hook's enable bcall.

To remove: call the corresponding disable bcall.

## Memory layout (per hook, 4 bytes)

- Bytes 0-1: hook routine address (little-endian)
- Byte 2: memory page number
- Byte 3: enable/disable flag (1 = active, 0 = inactive)

## Chaining

When multiple programs want the same hook:

- **Restoration approach:** save the original 4 bytes, install yours, restore on exit.
- **Chaining approach:** save the original hook data, install yours, jump to the saved address when your hook completes. Must maintain identical calculator state when invoking the chained hook.

### Warnings

- Avoid double-chaining loops (A chains to B which chains back to A).
- Returning values directly to the OS may justify skipping the chained hook call.

## Installation example (from the page)

```asm
#define hook_addr $1234

Install_hook:
    ld hl, hook_start
    ld de, hook_addr
    push de
        ld bc, hook_end-hook_start
        ldir
    pop hl
    bcall(_SetGetKeyHook)
    ret
hook_start:
    .db 83h
    ...
hook_end:
```

## Notes for our use case

- The `.db 83h` validation byte is the critical detail the dead-ends memory
  flagged as "hook return contract not in vendored references." This is the
  contract.
- Page 1 for RAM-based hooks. Our resident asm program (if/when it exists)
  can install hooks from RAM.
- Chaining matters if any other program (DCS, an app) is already using the
  same hook. For the silent-link case this is unlikely, but the
  GetCSC/Homescreen hooks are popular.
