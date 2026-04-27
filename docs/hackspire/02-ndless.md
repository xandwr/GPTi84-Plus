---
source: hackspire.org Ndless section
fetched: 2026-04-26
purpose: Native code execution on Nspire — toolchain, API, executable format, syscalls.
---

# Ndless Reference

## What it is

Open-source (MPL) loader + utilities that let third-party C/ARM-asm code run on TI-Nspire. Required to run anything that isn't a `.tns` document.

Latest stable: **ndless.me**. Source: GitHub (ndless-nspire/Ndless).

Multiple version paths documented (v1.0 → v1.1, ... v4.4 → r2005). New CX II OS releases periodically need a matching Ndless release.

## Toolchain

| Tool | Role |
|------|------|
| `nspire-gcc` | gcc wrapper for C/asm |
| `nspire-ld` | gcc + `-fuse-ld=gold` linker wrapper |
| `genzehn` | ELF-with-reloc → Zehn |
| `make-prg` | wraps Zehn with `zehn_loader.tns` for old Ndless |
| `nspire-tools new <name>` | scaffold a project Makefile |

Linux deps: git, gcc (C++), binutils, GMP, MPFR, MPC, zlib, boost-program-options, wget. Windows: WSL or Cygwin (32-bit, x86). MinGW/MSYS broken.

Setup: extend `PATH`, run `build_toolchain.sh`.

## Zehn executable format

- "Zehn" = German 10, vs. ELF = 11.
- Up to 16 MB.
- Full relocation (no `-fpic` needed).
- Embeds metadata: required Ndless rev, supported HW (Clickpad / Touchpad / CX / CM — CX II should be added), name/version/author/description.
- Header sig `ZEHN_SIGNATURE`, 4-byte aligned within first 4 KiB if file starts with `PRG\0` (back-compat shim).

Reference: `zehn.h` in Ndless repo.

## Features

- C and ARM/Thumb asm (only **ARM-state** `main()` entry; rest can be Thumb).
- File-extension association: register handlers, get file path via argv.
- Static libraries via `.ndless` directories.
- Resident programs: stay in RAM after exit (`nl_set_resident()`).
- Spawn other programs (`nl_exec()`).
- Detect startup vs. user launch (`nl_isstartup()`).
- Read/write OS variables via abstractions.
- Syscall surface to OS.

## Limitations (documented)

- Only ARM-state `main()`. (Crash on Thumb-state main.)
- Memory limits, sandbox, multitasking — **not explicitly documented**; treat as unbounded for our purposes.

## Syscalls

**Definition:** "OS functions exposed by Ndless to C and assembly programs."

Calling-convention details aren't on Hackspire — read Ndless source. Categories:

1. **C stdlib + POSIX (newlib)** — most things present. **`link()`, `fstat()`, `kill()` compile but misbehave.**
2. **Nucleus RTOS** filesystem/dir: `NU_Current_Dir`, `NU_Get_First/Next/Done`, `NU_Set_Current_Dir`. Interrupts: `TCT_Local_Control_Interrupts`.
3. **UTF-16 string API** — dynamic strings, codec.
4. **Graphic Context (GC) API** — shapes, text, images, blit.
5. **NavNet** — USB device-to-device + computer-to-calc transfer protocol. **Directly relevant** to bridge work — see OS Connectivity section.

Syscall numbers vary by OS version → Ndless maintains a mapping table.

## Debugging

- **nspire_emu** built-in ARM debugger — disasm, step, mem/instr breakpoints, backtrace, mem dump. `?` for help.
- **GDB**: `nspire_emu /G=3333`, then `arm-none-eabi-gdb` → `target remote localhost:3333`.
- **Eclipse CDT** wraps the GDB flow in a GUI.
- On-calc debug techniques: not documented; rely on `printf`-via-screen or UART.

`bkpt()` in libndls = software breakpoint (emulator only).

## libndls API

### Versioning
- `void assert_ndless_rev(unsigned)` — popup if too old.
- `const char *NDLESS_DIR` — path to ndless docs folder.

### LCD / framebuffer
- `scr_type_t lcd_type()`
- `bool lcd_init(scr_type_t)`
- `void lcd_blit(void *buf, scr_type_t)`

Screen types: `SCR_320x240_4` (4-bit gray), `_8` (8-bit pal), `_16` (RGB444), `_565` (RGB565), `SCR_240x320_565` (rotated RGB565).

### UI
- `show_msgbox(title, msg)` — 1-button.
- `show_msgbox_2b/3b(...)` — multi-button.
- `show_msg_user_input(title, msg, default, **out)` — text input.
- `show_1numeric_input(...)`, `show_2numeric_input(...)`.
- `refresh_osscr()` — refresh OS file browser after FS mutation.

### Input
- `any_key_pressed()`, `isKeyPressed(k)`, `on_key_pressed()`.
- `wait_key_pressed()`, `wait_no_key_pressed()`.
- `touchpad_getinfo()` → `touchpad_info_t *`.
- `touchpad_scan(touchpad_report_t *)`.
- `get_event(s_ns_event *)` — OS event poll.
- **`send_key_event/send_click_event/send_pad_event`** — synthesize events. **Critical for "trap the OS" / remote-control flow.**

### FS / CPU / timing
- `enable_relative_paths(char **argv)`
- `clear_cache()` — flush+inval ARM caches (needed after self-modifying code, dynamic load).
- `idle()` — wait-for-interrupt low power.
- `msleep(ms)`.

### HW detection
- Globals: `is_classic`, `is_cm`, `has_colors`, `is_touchpad`.
- `unsigned hwtype()`.
- `IO(addr1, addr2)` — pick HW-conditional reg address.

### Config
- `cfg_register_fileext(ext, prgm)` — file-association install.

## CAS programming

Phoenix OS supports CAS-style work like TI-68k AMS, but no AMS jump-table equivalent. Access via `primary_tag_list`. Low-level handles (`next_expression_index`, `push_quantum`, `top_estack`, `NG_control`) need per-OS-version address lookup. Quantum/ESQ are 2 bytes on Phoenix vs. 1 on AMS.

OS-integration story (read/write OS vars from Ndless, call Ndless from BASIC) is **explicitly undocumented**. Native EStack POC exists for Phoenix 1.7.2741 on ticalc.org.
