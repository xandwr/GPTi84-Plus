# 83Plus:Hooks:9B8C - Homescreen Hook

Source: https://wikiti.brandonw.net/index.php?title=83Plus:Hooks:9B8C

## Synopsis

- **Hook Name:** Homescreen Hook
- **Hook Pointer Block:** 9B8C
- **Enable BCALL:** 4FAB (`_SetHomescreenHook`)
- **Disable BCALL:** 4FAE (`_ClrHomeScreenHook`)
- **Call BCALL:** None known
- **Active Flag:** bit 4 of (iy + 34h)

## When it fires

Various homescreen events. The hook receives a condition code in A:

### A = 0 : Display Result
- OP1 contains the value to display.
- Modify OP1 to display alternate content without affecting Ans.
- For wide output, write formatted string to fmtString.
- Return NZ to suppress TIOS display.

### A = 1 : Key Press
- B contains the keycode.
- Modify B to simulate a different keypress.
- Return NZ to ignore the keypress.

### A = 2 : Expression Evaluation
- OP1 contains prgm!
- Use prgm# to retrieve expression.
- Return NZ to cancel program execution.

### A = 3 : Context Change
- B contains previous context value.
- Always return Z.

## Example

```asm
HomescreenHook:
    .db 83h            ; required for all hooks
    or a               ; condition 0?
    jr nz, ReturnZ
    bcall(_Random)     ; replace result with random number
ReturnZ:
    cp a               ; set Z
    ret
```

## Notes for our use case

This is the canonical "ambient status indicator on homescreen" hook per
project_os_integration_hooks.md. Specifically condition 0 (display result)
is the place to e.g. draw a "wireless connected" pixel or character before
the OS draws the result.

Not directly relevant to the calc-master REQ work, but THE relevant hook
for the OS-integrated UI items in project_os_integration_hooks.md.
