# 83Plus:Hooks:9BD0 - Silent Link Hook

Source: https://wikiti.brandonw.net/index.php?title=83Plus:Hooks:9BD0

## Synopsis

- **Hook Name:** Silent Link Hook
- **Hook Pointer Block:** 9BD0
- **Enable BCALL:** 50CE (`_SetSilentLinkHook`)
- **Disable BCALL:** 50D1 (`_DisableSilentLinkHook`)
- **Call BCALL:** Unknown
- **Active Flag:** bit 7 of (iy + 36h)
- **OS Requirement:** introduced in OS 1.13. Apps should check OS version first.

## Purpose

"Lets you abort a silent link request." It allows applications to *prevent*
silent link interruptions during operations like GetKey calls.

## Contract

Operates through the zero flag. Per the page: "set it to disable the silent
link handler."

## Credits

Michael Vincent is credited for documenting this hook.

## CRITICAL note for our use case

**This hook does the OPPOSITE of what we wanted.**

The dead-ends memory (`project_calc_to_pico_recv_dead_ends.md`) ranked
`_SetSilentLinkHook` as path 2: "real OS hook for exactly this case
[receiving silent link traffic during asm execution]." That premise is
WRONG.

This hook fires so the asm program can **abort** silent link requests, not
service them. Setting Z disables the silent-link handler entirely. This is
the hook used by programs that need to keep `_GetKey` from being interrupted
by background link traffic, e.g. games that don't want to be torn down when
someone plugs in a calculator.

For our problem, this hook is **the wrong tool**:
- It can't deliver inbound silent-link data to us.
- It runs from the OS's silent-link handler, but only to ask "should I keep
  going?" It doesn't expose the packet.
- The packet itself is processed (or aborted) by the OS regardless.

What this confirms about our architecture:

1. Silent-link receive IS interrupt-driven on the 84+ : the hook fires from
   the interrupt handler. So the link assist mechanism is real, the dead-end
   memory's "OS link service is polled, not interrupt" conclusion was based
   on `halt`-and-poll evidence which means halt-wake context isn't where
   that handler runs.
2. The OS will service silent-link traffic in the background regardless of
   whether asm is running, *unless* this hook says otherwise.
3. The `_GetKey`-unwinds-asm behavior the dead-ends memory observed is the
   OS's POST-receive policy, not the receive itself. The byte-level
   handshake completes (Pico log confirms this); the AppVar lands in NVRAM;
   THEN the OS decides "user app should yield" and unwinds.

**Updated path forward:** options ranked by feasibility:

1. **Calc-master REQ from `_SendPacket` primitives.** Unchanged. Bypasses
   the OS unwind by initiating from our side. Still the right answer.

2. **Silent-link hook + clever `_GetKey` interception.** New hypothesis: if
   the silent-link hook fires DURING the receive (before the OS decides to
   unwind us), we can use it as a flag-set ("CHATIN incoming, don't unwind
   us"). But the contract here only says abort/allow : we can't actually
   prevent the unwind from inside this hook unless aborting silent link
   entirely also prevents the unwind. UNTESTED hypothesis. Worth a 30-min
   experiment if calc-master REQ stalls.

3. **GetCSC hook + `_GetKey` poll.** Still works as the "render on next
   keypress" UX downgrade. UX worse than (1).

The dead-ends memory should be updated to reflect: option 2 (silent-link
hook) is misnamed in the original analysis : the actual hook does
abort/allow, not service. Its real value is unproven for our use case.
