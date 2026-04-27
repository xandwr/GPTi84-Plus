---
source: hackspire.org Operating System section
fetched: 2026-04-26
purpose: OS-side reference — USB protocol wire format, 84+ emulation, filesystem, NAND, Lua, documents.
---

# OS, Connectivity, and Storage Reference

## USB Computer Link — wire format

Calc advertises in standard mode:
- VID/PID: `0x0451 / 0xE012` (CX II = `0xE022`, per libnspire)
- Bus-powered, 100 mA
- Vendor-specific class
- **1 Bulk IN, 2 Bulk OUT** endpoints (different from 84+/Titanium)

### Packet format (16-byte header + payload)

```
54 FD  SA SA  SS SS  DA DA  DS DS  DC DC  SZ  AK  SQ  CK  [data...]
```

| Field | Size | Meaning |
|------|------|---------|
| `54 FD` | 2 | Magic |
| `SA SA` | 2 | Source addr |
| `SS SS` | 2 | Source service ID |
| `DA DA` | 2 | Dest addr |
| `DS DS` | 2 | Dest service ID |
| `DC DC` | 2 | Data CRC (proprietary) |
| `SZ` | 1 | Data size; `0xFF` if >254 |
| `AK` | 1 | `0x0A` for ACK packets, `0x00` otherwise |
| `SQ` | 1 | Sequence # (1–255, never 0) |
| `CK` | 1 | Header checksum (sum of preceding bytes mod 256) |

Flow: Host Req → Dev ACK → Dev Resp → Host ACK. Host always initiates after addr assignment.

### Address assignment
USB reset → device sends addr-request packet → host replies with assignment. Both sides keep their own addr + sequence counters.

### Service IDs

| ID | Service |
|----|---------|
| 0x4002 | Echo |
| 0x4020 | Device info |
| 0x4024 | Screenshot (RLE) |
| 0x4060 | File mgmt (dir, transfer) |
| 0x4080 | OS install |

### File commands (within payload)
- `0x03` Put file
- `0x07` Get file
- `0x0D / 0x0E / 0x0F` Dir enum init/next/done

File contents are split into 253-byte chunks (max 254 byte payload − 1 cmd byte).

### Screenshot
RLE: signed length bytes — non-negative = run length−1, negative = literal run. 4-bit grayscale pixels (Classic).

### OS install
Send file in chunks → calc continuously reports progress 0..0x65 until max.

### CX II differences
Hackspire's USB Protocol page predates CX II detail — for `0xe022` specifics, **read `vendor/libnspire-rs/libnspire-sys/libnspire/src/cx2.cpp`**, which is the authoritative source.

## Connectivity (loopback / NavNet)

- "Send OS" with no host present causes the calc to simulate send+recv simultaneously — local file consumed, no USB traffic.
- Document transfers via context menu behave the same; received file gets numerical suffix.
- **NavNet is the lab/cradle wireless protocol** — exposed via Ndless syscalls (see Ndless syscall categories) but not documented further on this Hackspire page.

Hackspire's Connectivity page does NOT cover Student Connect / Student Software auth flow. That has to be RE'd from firmware or captured live.

## TI-84 Plus Emulation (84-mode on Nspire)

Triggered by **swapping the Nspire keypad for the TI-84+ keypad** (physical detection on Nspire models that supported it; CX II does **not** support 84 mode).

### Storage
- 84+ OS lives inside Nspire OS upgrade, certificate field `8070`, encrypted/compressed.
- Archive memory = 64 KB files in a PK-Zip inside that blob.
- Boot code stays at 84+ v1.02; emulated OS bumps even versions only (2.42, 2.44, 2.46).

### Z80 emulation
- Custom opcodes intercept link/Flash/keypad-lock — these replace 84+'s memory-mapped Flash protocol with "invalid instructions" trapped by the host.
- Self-test combo disabled. OS-via-link disabled. Programs using `in 0,(c)` or IXH/IXL fail.

### LCD mapping
- 96×64 native → mapped to 288×192 area (3×3 Nspire pixel per 84 pixel).
- Bug: contrast resets after low-power halt + reactivation.

### USB / Flash persistence
- USB events **not propagated** into emulated 84 — TI integrated DBUS instead. No USB-using 84 software works.
- Writes to user archive (pages 0x08–0x69) persist in Nspire FS.
- Writes to OS pages **NOT preserved** — restored on keypad re-insert.

**Implication:** TI's keypad-swap-into-84-mode IS the official "speak DBUS" path. Our existing 84+ DBUS work is reusable for any Nspire that has a swappable 84+ keypad — but the **CX II we own doesn't have one** (per project memory).

## Internal Filesystem

Two-layer: **FlashFX Pro** (wear leveling, bad-block) on the bottom, **Reliance** filesystem on top.

### FlashFX Pro
- Maps logical → physical pages.
- Each unit's first block has a 0x34-byte header: magic `0x48E2`, client addr, erase count, serial #, sequence #, CRC.
- Spare area (1/32 of page) carries allocation + ECC.
- Latest logical-page version = greatest seq #, then greatest physical #. Unallocated = `0xFF`.

### Reliance
- Block-pointer based (offset = block# × blockSize).
- **MAST** header at offset 0x40: magic `MAST`, block size/count, META pointers, vol create date, IMAP block count, CRC.
- **META** header (two copies, counter picks current): index block ptr, free-block tracking, IMAP, stats.
- **INOD** (inode): index, size, ctime/mtime/atime (ms), attrs.
  - Attr low 2 bits: 0=inline data, 1=ptrs to data blocks, 2=ptrs to INDI, 3=ptrs to DBLI.
- Directory entries: marker `0x80`, CRC, entry len, name len, attrs, inode ptr, name as 16-bit chars.
  - Attr mask: `0x1` = in-use, `0x2` = directory.
- Inode 0 = INDX magic, 1 = index file, 2 = root dir, 3–4 = bad-file/vol-label.

## NAND layout

Page sizes:
- Original Nspire: 528 (512+16 OOB)
- **CX / CM / CX II: 2112 (2048+64 OOB)**

### Classic / CX / CM (page-addressed)

| Region | Pages (Classic) | Pages (CX/CM) | Role |
|--------|----------------|----------------|------|
| Manuf | 0x0000–0x001F | 0x0000–0x003F | Mfg data |
| Boot2 | 0x0020–0x0A7F | 0x0040–0x057F | Boot loader |
| Bootdata | 0x0A80–0x0AFF | 0x0580–0x063F | Cycling config |
| Diags | 0x0B00–0x0F7F | 0x0640–0x079F (CX) / 0x07BF (CM) | Diag SW |
| Diags Results | 0x0F80–0x0FFF | 0x0780–0x07FF (CX) | Test results |
| FS | from 0x1000 | from 0x0800 (CX) / 0x07C0 (CM) | Files + factory images |

### CX II (block-addressed, 64-page blocks)

| Name | Blocks | Offset (blocks) |
|------|--------|------|
| Manuf | 1 | 0 |
| Bootloader | 4 | 1 |
| PTT Data | 1 | 5 |
| Diags | 5 | 29 |
| OS file | ? | 36 |
| Logging | 87 | 114 |

**Key:** OS lives in its **own partition**, not the FS — distinct from CX/Classic.

## OS upgrade files (`.tno` / `.tnc`)

- PK-Zip archive with custom ASCII header (terminated by `0x1A`).
- Header carries: filename, version, total size, resource info, decompressed FS size — used for pre-install size check + post-transfer integrity.
- Archive members:
  - `TI-Nspire.cer` — RSA pubkey cert
  - `TI-Nspire.img` — OS image (code + FS + signature); may be Blowfish/3DES encrypted, compressed, or zipped containing `image.bin`
  - `boot2.cer` / `boot2.img` (OS 1.4+) — boot loader update

### Verification chain
- Boot1 → validates Boot2 (RSA-1024 / RSA-2048, SHA-256 sig)
- Boot2 → validates OS image
- All sigs SHA-256 + RSA

**Implication:** modifying the OS image requires either signing-key extraction (almost certainly impossible) or pre-Boot2 exploitation. **Ndless must achieve all goals in user-mode** post-OS load.

## Lua programming

- Available since OS v3.0, official support v3.0.1.
- Scripts run inside `.tns` document model (XML wrapper + Lua).
- XML metadata: `TARAL` (API level), `TITLE`, script-version attr.
- Full API documented externally on **Inspired Lua Wiki** (Hackspire defers).

**Use case for our project:** Lua scripts can be the *plausibly innocent* container for code that reaches out to a hardware peer. They run inside the OS sandbox so they can't drive USB / GPIO — but they CAN render UI, accept input, and call certain TI APIs. Useful as the front-end UI of a "trap-the-OS" disguise.

## Document management

- `.tns` = PK-Zip with `Document.xml` + `Problem<N>.xml`.
- `Document.xml` mirrors OS Document Settings dialog.
- Validation is loose (extension only) — but compression method matters: GUI 7-Zip works, CLI / Windows-native compression breaks.
- Ill-formed XML → error popup. Invalid widget type → empty frame. Stray text outside tags → ignored. Mod problem-version attr → error.
- Files >25 KB uncompressed → calc freezes briefly before erroring.
- Variable types: lists, ints, floats, strings, functions; var names ≤16 chars.
