# 83Plus:Hooks:9B78 - Link Activity Hook

Source: https://wikiti.brandonw.net/index.php?title=83Plus:Hooks:9B78

## Synopsis

- **Hook Name:** Link Activity Hook
- **Hook Pointer Block:** 9B78
- **Enable BCALL:** 4F84 (`_EnableLinkActivityHook`)
- **Disable BCALL:** 4F87 (`_DisableLinkHook`)
- **Call BCALL:** 4F81 (`_call_linkactivity_hook`)
- **Active Flag:** bit 4 of (iy + 33h)
- **Override Flag:** bit 3 of (iy + 33h) : when set, hook does not execute

## When it fires

"Whenever link activity is detected by the OS's interrupt handler." Triggers
specifically when either of the two data lines on the link port is pulled
low.

## Contract

**Input:**
- A = value read from the link port, upper 6 bits masked out (bottom 2 bits only)

**Output:**
- All values and flags returned by the hook are **ignored** by the OS

## Critical limitation

Bit 3 of (iy + 33h) is the override flag : when set, the hook does NOT
execute. This bit is managed automatically during GetKey operations, which
prevents the hook from firing throughout most of the TI-OS.

> "It is impossible (without using another hook to reset the flag during
> GetKey) to receive events during GetKey; this means this hook can't be
> triggered throughout most parts of the TI-OS."

## Notes for our use case

This is closer to what we wanted but has critical flaws:

**Pros:**
- Fires from interrupt handler whenever link lines move : true real-time.
- Confirms the link IS interrupt-capable on the 84+ (refines the dead-ends
  memory's polled-vs-interrupt conclusion).

**Cons:**
- **Output is ignored.** This is purely an "FYI, link line moved" notification,
  not an opportunity to intercept or service the packet.
- **Suppressed during GetKey.** Bit 3 of (iy+33h) blocks the hook during
  `_GetKey` runs : exactly the context our CHAT is in when `await_response`
  is called.
- Documented workaround: install a *second* hook that clears the override
  bit during GetKey. That's two hooks deep and starting to feel like
  fighting the OS.

**As a useful primitive:** could be used as a "the user plugged something
in, set a flag" notifier for a CHAT idle state. Not a path to receiving
data. The actual data lands via the OS's silent-link receive code, not via
this hook.

## Verdict

Doesn't replace calc-master REQ for our use case. Worth knowing about for
ambient UI work (e.g. light up a status indicator when link activity
happens), per project_os_integration_hooks.md.
