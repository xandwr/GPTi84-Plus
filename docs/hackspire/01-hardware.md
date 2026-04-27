---
source: hackspire.org Hardware section
fetched: 2026-04-26
purpose: Architectural reference — register addresses and hardware blocks for TI-Nspire CX II (primary target) and CX (older variant).
---

# Hardware Reference

## CX II Memory-Mapped I/O (PRIMARY TARGET)

| Address | Block | Notes |
|---------|-------|-------|
| 0x00000000 | Boot1 ROM | 128 KB on-chip |
| 0x10000000 | SDRAM | 64 MiB, controller at 0x90120000 |
| 0x90000000 | GPIO | 5 sections × 8 pins |
| 0x90010000 | Fast Timer | Same iface as 0x900C/0x900D |
| 0x90020000 | UART | PL011 |
| 0x90030000 | Fastboot RAM | 4 KiB, **survives reset** — sleeper state stash |
| 0x90040000 | SPI (LCD) | FTSSP010 |
| 0x90050000 | I2C | Synopsys DesignWare, touchpad bus |
| 0x90060000 | Watchdog | SP805 |
| 0x90070000 | UART #2 | PL011 |
| 0x90080000 | Cradle SPI | FTSSP010, EEPROM |
| 0x90090000 | RTC | PL031-like |
| 0x900A0000 | Misc / Model ID | 0x202 = CX II |
| 0x900B0000 | ADC | FTADCC010 |
| 0x900C0000 | Timer #1 | SP804 + speed ctrl @ +0x80 |
| 0x900D0000 | Timer #2 | SP804 + speed ctrl @ +0x80 |
| 0x900E0000 | Keypad | Scan / IRQ / data |
| 0x90120000 | DDR3 ctrl | FTDDR3030 |
| 0x90130000 | LCD backlight | brightness +0x14 (0–225), enable +0x18 (write 255) |
| 0x90140000 | PMU | Aladdin; sleep/wake; ON key bit 8 of 0x90140810 |
| 0xA0000000 | Boot1 mirror | — |
| 0xA4000000 | Internal SRAM | 0x40000 bytes |
| 0xA8000000 | Magic VRAM | 0x25800 bytes; HW X/Y swap & rotate |
| 0xAC000000 | SDIO | **WiFi card / NAND diag** — radio surface |
| 0xB0000000 | USB OTG (top) | FOTG210 — visible Mini-B |
| 0xB4000000 | USB OTG (dock/bottom) | FOTG210 — **stealth port** |
| 0xB8000000 | SPI NAND | FTSPI020 + Micron 1 Gb |
| 0xBC000000 | DMA | FTDMAC020 (PL080-like) |
| 0xC0000000 | LCD ctrl | PL111 |
| 0xC8010000 | 3DES | 3-key block engine |
| 0xCC000000 | SHA-256 | HW hash |
| 0xDC000000 | IRQ ctrl | PL190 |

## CX (older, non-II) — for reference

Differences from CX II worth noting:
- 32 MiB SDRAM on CM, 64 MiB on CX
- LED at 0x90110000 (red/green, blink patterns) — **not present on CX II**
- HDQ/1-Wire at 0x900F0000 (TI OMAP-style)
- SDRAM ctrl at 0x8FFF0000 (DMC-340), NAND ctrl at 0x8FFF1000 (PL351)
- USB host EHCI at 0xB4000000 (vs. dual FOTG210 on CX II)
- ADC at 0xC4000000 (vs. 0x900B0000)
- Power mgmt at 0x900B0000 (CX) — moved to 0x90140000 on CX II
- Keypad ON-key bit 4 of 0x900B0028 (CX/CM) vs. bit 8 of 0x90140810 (CX II)

## GPIO

5 sections at 0x90000000–0x9000017F (sections 0,1,2,3,5; **note section 4 is missing**). Each section = 8 pins. Pin number = `section*8 + bit`.

Per-section register layout (offsets from section base):

| Off | R/W | Function |
|-----|-----|----------|
| +00 | R | Masked IRQ status ([+04] & [+08]) |
| +04 | R | Raw or sticky IRQ status (mode at +20) |
| +04 | W | Reset sticky bit (write 1) |
| +08 | R | IRQ mask state |
| +08 | W | Enable IRQ (set mask) |
| +0C | W | Disable IRQ (clear mask) |
| +10 | R/W | Direction (0=output, 1=input) |
| +14 | R/W | Output value |
| +18 | R | Input value |
| +1C | R/W | Invert raw IRQ status |
| +20 | R/W | Sticky vs raw IRQ select |
| +24 | R/W | Unknown |

Known assignments (Classic / CX — CX II not yet documented on Hackspire):

| Pin | Classic | CX |
|-----|---------|----|
| 0:1 | I2C clock (touchpad) | — |
| 0:2 | Battery door (0=open) | USB VBUS ctrl (active-low) |
| 0:3 | I2C data (touchpad) | — |
| 0:5 | — | USB VBUS ctrl (active-low) |
| 0:6 | — | USB charge ctrl (active-low) |
| 2:19 | — | WLAN cradle present (0=attached) |
| 2:20 | — | USB micro-B detect (1=attached) |
| 2:23 | — | LCD power (LCD_OFF in diag mode) |
| 3:24 | Reset button (0=pressed) | Keypad present (1=not plugged) |

## Keypad

Mapped 0x900E0010–0x900E001F. Halfwords, bits 0–10 = keys.

ON/HOME key:
- Touchpad/CX/CM: bit 4 of `0x900B0028`
- **CX II: bit 8 of `0x90140810`**

Wiring: 30-pin connector, alternating Col/Row pattern (pins 3–30) + GND + Vcc. Cols → input pins, Rows → output pins. Scan = drive one row, read all cols.

Key-bit polarity:
- Clickpad: bit **clear** = pressed
- Touchpad / CX / CM / **CX II**: bit **set** = pressed (inverted)
- TI-84+ keypad: layout differs (offset 0010 = directional cluster)

`libndls`: `on_key_pressed()` abstracts model differences. `iskeypressed()` is the per-key check.

### Touchpad I2C protocols (CX II HW rev relevant)

- Pre-CX-II HW rev. AK: **Synaptics** at I2C addr 0x20. Multi-page; page 04 contact status, X/Y BE16, pressed at port 0x0A.
- CX II HW rev. AK+: **CapTIvate** custom. Cmd 0x01 → flags (bit 0=pressed, bit 1=touched) + X/Y LE16.

## Interrupts

PL190 controller at 0xDC000000 (CX/CX II).

| Offset | Function |
|--------|----------|
| +0x000 | Masked status |
| +0x004 | Raw/sticky status |
| +0x008 | Mask enable (write 1 to enable) |
| +0x00C | Mask disable (write 1 to disable) |
| +0x020 | Current IRQ number |
| +0x024 | Read current IRQ number (handler) |
| +0x028 | Acknowledge / reset trigger |
| +0x02C | Max priority |
| +0x204 | Control |
| +0x300–0x37F | Priority array |

ARM modes: IRQ (vector @ 0x18, CPSR I=0x80) / FIQ (vector @ 0x1C, CPSR F=0x40). Return: `SUBS PC, LR, #4`.

Known IRQ sources (Classic numbering — assumed similar on CX/CX II):

| # | Source |
|---|--------|
| 1 | UART |
| 3 | Watchdog |
| 4 | RTC |
| 7 | GPIO |
| 8–9 | USB controllers |
| 11 | ADC |
| 13 | SD host |
| 14 | HDQ / LCD |
| 15 | Power mgmt |
| 16 | Keypad |
| 17–19 | Fast / Timer1 / Timer2 |
| 20 | I2C |
| 21 | LCD controller |
| 22 | TI-84 link port |

Handler flow: save regs → read IRQ# at +0x24 → optionally clear sticky → write +0x28 to reset → handle source → ack source → restore → exit.

## Timers

Three timer modules. Speed-control register at base+0x80 (Fast Timer documented):
- bit 0: ~10 MHz clock
- bit 1: 32 kHz (overrides bit 0)
- neither: 33 MHz default

Hackspire timer page is thin — full SP804 datasheet covers register set.

## Virtual Memory / MMU

ARM926EJ-S MMU, **1 MB sections**, 4096-entry table (CP15 c2 = TTBR). Coarse protection — listed sections allow R+W, others trap.

Map highlights:
- Internal SRAM 0x00000000–0x000FFFFF
- SDRAM 0x10000000–0x11EFFFFF (write-back cached)
- Peripherals from 0x8FF00000 (uncached, unbuffered)
- Last 1 MB of SDRAM uncached/unbuffered
- Internal RAM aliased at 0xA4000000

No documented kernel/user split — flat model. Ndless interaction with MMU not detailed on this page.

## Clock / PLL

Configured via 0x900B0000 first two regs (CX) — moved to 0x90140000 on CX II (power-mgmt block).

CX (48 MHz base):
- bits 0–5: Base/CPU
- bits 6–11: CPU/AHB
- bits 12–20: Base val
- bits 21–30: unknown multiplier (1 or 2)

Calc chain: CPU = 48 MHz / Base-to-CPU; AHB = CPU / CPU-to-AHB; APB = AHB / 2.

Classic (27 MHz): base = 300 − 6×Baseval, same divider chain.

CX II clock control register layout: not documented on Hackspire — RE'd via libnspire / firmware needed if overclocking matters.
