---
source: hackspire.org (full-site crawl, Apr 2026)
purpose: Project-relevant synthesis of Hackspire — what matters for ti84-superdeluxe and why.
---

# Hackspire Reference — Project Synthesis

Local distillation of [hackspire.org](https://hackspire.org) into the parts that actually matter for the CX II sleeper-bridge work. Per-section detail lives in the sibling files; this doc is the index + the "so what."

## Index

- [01-hardware.md](01-hardware.md) — register maps (CX II + CX), GPIO, keypad, IRQ, timers, MMU, PLL.
- [02-ndless.md](02-ndless.md) — Ndless toolchain, Zehn format, libndls API, syscalls, debugging.
- [03-os-and-comms.md](03-os-and-comms.md) — USB Computer Link wire format, NavNet, 84-mode emulation, Reliance FS, NAND layout, OS upgrade format.
- [04-misc-and-ecosystem.md](04-misc-and-ecosystem.md) — Firebird emulator, Linux port, existing native programs, Z80 special opcodes, Jazelle, TI.Image, site history.

## What's load-bearing for the project

### 1. The dock USB port is real and stealth

CX II has **two** FOTG210 USB OTG controllers — top mini-B (visible) at `0xB0000000` and **bottom dock connector** at `0xB4000000` ([01-hardware.md](01-hardware.md)). Public Faraday IP, register interface documented. A dongle on the dock is the cleanest external-normality path — nothing on top of the calc, nothing on screen.

### 2. SDIO + on-die WiFi support is a legit second path

`0xAC000000` SDIO controller is wired to "WiFi card and diagnostics NAND imaging" ([01-hardware.md](01-hardware.md)). Combined with the **Student Connect** UI we observed live on the calc, the OS already has a wireless stack. Two unknowns to investigate before committing:

1. Does Student Connect bind to the lab cradle SSID/AP only, or will it associate with a generic WPA2 AP?
2. What's the auth shape on the wire — TI-proprietary, EAP, or plain HTTPS?

If unmodified Student Connect can talk to a generic AP, the bridge problem may collapse to "stand up an AP that proxies to the iPhone." That's a much shorter path than building NavNet-over-USB ourselves.

### 3. libnspire ≠ Hackspire — and that's fine

Hackspire's USB Protocol page documents original Nspire (`0xe012`) only; CX II divergences live in `vendor/libnspire-rs/libnspire-sys/libnspire/src/cx2.cpp`. The Hackspire wire format (16-byte header, services 0x4002/0x4020/0x4024/0x4060/0x4080) is the *base* layer; cx2.cpp documents the deltas. Read both, don't pick one.

### 4. The 84-emulation path is dead **on this calc**

CX II doesn't accept the swappable 84+ keypad. The Hackspire 84-emulation docs ([03-os-and-comms.md](03-os-and-comms.md)) are useful as architectural reference (they validate that our 84+ DBUS firmware speaks the right protocol), but the keypad-swap-into-84-mode trick doesn't apply. Existing ESP-IDF DBUS firmware in `main/link_dbus.c` stays as proof-of-architecture only.

### 5. Backlight covert channel is real, with a caveat

`0x90130014` brightness (0–225) and `0x90130018` enable ([01-hardware.md](01-hardware.md)) are software-controlled. A photodetector on the screen could read sub-perceptible PWM. Caveat: an Ndless module modulating the backlight is **only** running if Ndless is loaded — not a cold-boot covert channel. Useful as low-bandwidth ack/heartbeat once the payload is alive.

### 6. Persistent 4 KB RAM survives reset

`0x90030000` Fastboot RAM, 4 KiB, **not cleared on reset** ([01-hardware.md](01-hardware.md)). Sleeper state stash that doesn't wear NAND. Use cases: pending-display payloads, command queue, "did the proctor reset me?" flag, exfil buffer between reboots.

### 7. RSA verification chain is sealed

OS upgrade verification is RSA-1024/2048 + SHA-256, Boot1 → Boot2 → OS ([03-os-and-comms.md](03-os-and-comms.md)). Modifying the OS image requires either signing-key extraction (effectively impossible) or a pre-Boot2 exploit. **Conclusion: all our payload work has to happen post-OS-load via Ndless.** No custom firmware path; we live inside the user-mode runtime Ndless gives us.

### 8. Ndless gives us enough

libndls ([02-ndless.md](02-ndless.md)) exposes `send_key_event`, `send_click_event`, `send_pad_event` — synthesize input events to drive the OS UI. Plus full LCD blit, framebuffer access, msgbox, file browser refresh. Combined with the syscall surface (Nucleus FS, GC API, NavNet), this is enough surface to build a "trap-the-OS" shell that:

- intercepts keypress events before the OS sees them
- renders a fake calculator UI while running Lua/whatever in the background
- exfils via SDIO/USB/backlight when the bridge polls

Not yet documented on Hackspire: how Ndless interacts with OS multitasking. To verify, read Ndless source `nspireio/` and `n2DLib/` — they're the closest references.

### 9. Reliance FS = persistence target

`.tns` files live in the Reliance filesystem on top of FlashFX Pro ([03-os-and-comms.md](03-os-and-comms.md)). Inode types include indirect block lists. **Hidden files via inode-attribute mod is plausible** — write a `.tns` whose dir entry has the dir bit set but the in-use bit clear, or with a name encoded such that the OS file browser skips it. Sleeper persistence target.

### 10. ndless.cfg + file-extension association = autorun

`cfg_register_fileext(ext, prgm)` (libndls) registers a handler. Combined with `nl_isstartup()` + `nl_set_resident()`, an Ndless module can:
- install itself on first run as the handler for some extension (or hijack `.tns`)
- mark itself resident
- detect startup vs. user launch
- stay loaded across reboots if Ndless is auto-launched (which it is, post-install)

This is the autorun chain. Pair with the Reliance hidden-file trick for full sleeper persistence.

## Open questions Hackspire doesn't answer

1. **CX II memory map gaps.** Hackspire admits it's incomplete. The clock-control register is documented for CX (48 MHz base, bit fields in 0x900B0000) but not CX II — moved to the 0x90140000 PMU block but no field map. RE from libnspire / firmware if overclocking matters.
2. **CX II GPIO assignments.** Section/pin-level docs are CX-only. Have to probe with an Ndless GPIO sniffer to find: keypad-rev signal, USB role-switch, dock-port role-switch, charge enable, etc.
3. **Student Connect protocol.** Not documented anywhere on Hackspire. Need either a Wireshark capture against the lab cradle (which I don't have) or static RE of the OS binary (the Student Connect tab has to call something).
4. **Ndless multitasking semantics.** Whether an Ndless resident program can co-exist with the OS event loop without freezing it. Mentioned indirectly via `nl_set_resident` but the model isn't documented. Read the Ndless source.
5. **CX II syscall numbers.** Hackspire's syscall page lists categories, not numbers. Per-OS-version mapping lives in the Ndless source tree (`syscalls.h` per OS version).

## Where to go next inside our codebase

- For protocol detail: `vendor/libnspire-rs/libnspire-sys/libnspire/src/{packet,service,cx2,usb}.c{,pp}` and `services/*.c`. Authoritative for CX II.
- For Ndless internals (not on Hackspire): clone `ndless-nspire/Ndless` from GitHub. Important headers: `zehn.h`, `nspireio.h`, syscall tables.
- For Linux-on-Nspire USB drivers (if going calc-as-bridge): `ti-nspire` device tree in mainline + the Linux-on-Nspire kernel tree.
