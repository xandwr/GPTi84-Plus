---
source: hackspire.org misc / projects / emulators / linux / etc.
fetched: 2026-04-26
purpose: Ecosystem reference — emulators, Linux port, existing native programs, Z80 special opcodes, Jazelle, image format.
---

# Ecosystem & Misc

## Emulators

### Firebird (current)
- Active. Native Windows/Linux/Mac/Android/iOS via Qt GUI.
- Supports Classic / Touchpad / CX / **CX II** (CX II requires decryption keys via **eMMUlate** — extracted from a real CX II's eMMC).
- Boot files dumped via **PolyDumper** (run on real calc, exfils boot1/boot2/manuf via link).
- Can create blank flash images.
- GitHub releases.

### Obsolete / abandoned
- **nspire_emu** — Windows only (Wine OK). First-gen. Clickpad/TouchPad/LabStation/CX(CAS). Built-in ARM debugger (`?`).
- **Ncubate** — nspire_emu fork, broken on OS v3.x. Added state save/load, more debugger commands, **GDB stub**.
- **Xspire** — nspire_emu GTK port, Win/Linux, skin support.
- **Nspire Memory Editor** — nspire_emu plugin: hex edit, search, mem dump, instr/string breakpoints.

**For our project:** Firebird with eMMUlate'd keys is the dev loop for any CX II Ndless work — flashes, OS, and our payload all run in emulator first.

## Linux on Nspire

Two ports:
- **Device-tree kernel** (newer, mainline-based). Works: CPU, GPIO, UART1, I2C (WIP), watchdog, both timers, keypad, touchpad (WIP), **USB OTG**, LCD. Missing: SRAM, RTC, PMU, LCD contrast/backlight, NAND.
- **Legacy kernel** (unmaintained). Classic: full coverage incl. SDRAM/SRAM/RTC/touchpad/ADC. CX: similar but USB-OTG support **breaks USB host**; NAND WIP.

Boot: `linuxloader2` runs from Nspire OS, copies kernel + initrd to RAM, jumps. Rootfs options: initrd or USB-drive FS (Arch Linux ARM, Debian).

Caveats:
- After Linux runs, touchpad misbehaves under Nspire OS without recovery.
- Calc's USB provides minimal power → may need powered hub.
- Legacy kernel can't switch USB host/device on the fly — must be preset in Nspire OS.

**For our project:** Linux port is the proof that USB host works on Nspire ARM hardware. If we wanted "calc as the bridge" instead of an external Pico, an Ndless module driving the FOTG210 directly is plausible — Linux drivers already do it.

## Existing native programs (relevant subset)

- **Nleash** — bypasses OS 2.1 downgrade lock. (Old, but: shows OS-version-gating bypass is feasible.)
- **Norse** — wireless-ish chat using **Press-to-Test LED** as a Morse-code TX. Conceptual neighbor to "covert backlight signaling" idea.
- **Nover** — overclock from OS 2.1 onward.
- **PolyDumper** — extracts boot1/boot2/manuf for emulator setup.
- **mViewer / nwriter** — basic file viewers, useful as Lua/Ndless UI templates.
- **nDoom / gbc4nspire / NES** — proves heavy ARM workloads run fine.

Games (Lua): Bobby Carrot, Breakout, Checkers, Cyberbox, Falling Blocks, FreeCell, Klondike, LabyRoll, ImprovedSoloPong, nFighter, nGolfe, nspire block dude, Nyan Cat, Pegs, Pacman, RayCaster, Reversi, TI-Basket, Space Invaders, Video Poker. (Need OS ≥ 3.0.1.)

## Z80 special instructions (84+ emulation)

The Nspire's Z80 interpreter intercepts invalid Z80 opcodes. Three two-byte prefixes:

- `ED ED ...` — most common; data byte selects op
- `ED EE` — Flash sector erase (replaces `_EraseFlash`)
- `ED EF` — Flash write (replaces `_WriteFlash`)

`ED ED <op> 10` opcode partial table:

| Op | Function |
|----|----------|
| 02 | Pre-shutdown prep |
| 03 | Lock to 84+SE keypad only |
| 04 | Unlock either keypad |
| 06 | Link byte recv |
| 0C–0E | USB / I/O bulk |
| 10–11 | Bulk out init |
| 12–13 | USB init |
| 17–1A | USB cfg / status |
| 1D–1F | Port 0xA0 out |
| 20, 24, 27 | USB status w/ callback |
| 22 | Crystal timer replacement (BC = input) |
| 29 | USB autolaunch check |
| 2F, 30 | Battery / reset |

> "Some of these will reboot the Nspire if run from Flash… horribly incomplete."

**Implication:** the 84+ DBUS path on Nspire 84-mode is faithful to the underlying TI link protocol. Our existing 84+ DBUS firmware would work against a keypad-swappable Nspire — but **not against the CX II we own.** Useful only as architectural fallback / reference.

## Jazelle (ARM Java-bytecode mode)

ARM926EJ-S has Jazelle: subset of Java bytecodes execute in HW.

Enable: set JE bit in Main Cfg, CV in OS Ctrl, point r5 at 1024-byte-aligned handler table.

Reg use during Java exec: r0–r3 stack words, r4 = local-var-0 cache, r6 = stack ptr (grows up), r7 = local-vars base, LR = Java PC.

Handler table covers unhandled bytecodes, NPE, array index, cfg errors.

Spec partly proprietary (DDI 0225A unavailable).

**For our project:** likely irrelevant. Mentioned only because Jazelle is an exotic execution mode that an Ndless module could theoretically abuse for code obfuscation against static analysis.

## TI.Image format (Lua image format)

20-byte header (little-endian):

| Off | Size | Field |
|-----|------|-------|
| 0 | 4 | Width |
| 4 | 4 | Height |
| 8 | 4 | Reserved (zero) |
| 12 | 4 | Buffer-row bytes (typically 2×width) |
| 16 | 2 | Depth (typically 16 = 15-bit + alpha) |
| 18 | 2 | Compression(?) — defaults to 1 |

Pixel format: 2 bytes, layout `ARRRRRGG GGGBBBBB`. A=opaque flag.

Encoding tip in Lua: bytes can be `\NNN` decimal escapes or a printable ASCII char to save document size.

## Retroengineering page
Hackspire's "Retroengineering" page is **legal-justification only** — French L122-6-1 + DMCA interop carve-outs. No technical RE content there. RE techniques are spread across the per-component pages (USB, NAND, OS upgrade, etc.) and the Ndless source.

## Site / community

- Hackspire founded July 2007 by **Squalyl**, took over by **Jim Bauwens** in March 2016.
- Source on GitHub: **beyond-ndless/hackspire.org** (PRs welcome).
- Discord/forums: yAronet, United TI, **Omnimaga** (active community for Lua + native programs), ticalc.org.

## Project misc

- TI internal codename: **Phoenix** (Computer Link app called "Phoenix Connect"). Useful string to grep for in firmware.
- TI considered shipping a Ruby interpreter. Didn't ship. Useful only as trivia.
- Devs: Zac Bowling (TI), Hydrix, LearningSoft committers in CVS logs. Toolchain assumed Eclipse on Mac.
