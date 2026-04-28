# WikiTI mirror (subset)

Source: https://wikiti.brandonw.net/

Fetched 2026-04-28 for the calc-master REQ work. Each file is a verbatim
reproduction of the corresponding WikiTI page (per WebFetch summarization;
re-fetch the live page if precise wording matters for an edge case).

## Index

### Link bcalls (page 1Bh)

| File | bcall | Address |
|---|---|---|
| [bcall_4ED6_SendPacket.md](bcall_4ED6_SendPacket.md) | _SendPacket | 4ED6 |
| [bcall_4EEB_GetSmallPacket.md](bcall_4EEB_GetSmallPacket.md) | _GetSmallPacket | 4EEB |
| [bcall_4EDC_Send4BytePacket.md](bcall_4EDC_Send4BytePacket.md) | _Send4BytePacket | 4EDC |
| [bcall_4EEE_GetDataPacket.md](bcall_4EEE_GetDataPacket.md) | _GetDataPacket | 4EEE |
| [bcall_4EE5_SendAByte.md](bcall_4EE5_SendAByte.md) | _SendAByte | 4EE5 |
| [bcall_4EDF_SendDataByte.md](bcall_4EDF_SendDataByte.md) | _SendDataByte | 4EDF |
| [bcall_4EF4_Get4Bytes.md](bcall_4EF4_Get4Bytes.md) | _Get4Bytes | 4EF4 |
| [bcall_4EF1_SendAck.md](bcall_4EF1_SendAck.md) | _SendAck | 4EF1 |
| [bcall_4ED9_ReceiveAck.md](bcall_4ED9_ReceiveAck.md) | _ReceiveAck | 4ED9 |
| [bcall_4ED3_SendRAMCmd.md](bcall_4ED3_SendRAMCmd.md) | _SendRAMCmd | 4ED3 |
| [bcall_4F06_ReceiveVar.md](bcall_4F06_ReceiveVar.md) | _ReceiveVar | 4F06 |
| [bcall_4A14_SendVarCmd.md](bcall_4A14_SendVarCmd.md) | _SendVarCmd | 4A14 |
| [bcall_4F4E_CheckLinkLines.md](bcall_4F4E_CheckLinkLines.md) | _CheckLinkLines | 4F4E |
| [bcall_50E3_LinkStatus.md](bcall_50E3_LinkStatus.md) | _LinkStatus | 50E3 |
| [bcall_4ECA_CancelTransmission.md](bcall_4ECA_CancelTransmission.md) | _CancelTransmission | 4ECA |
| [link_bcalls_index.md](link_bcalls_index.md) | (index) | full Category:Link list |

### Hardware / RAM

| File | Topic |
|---|---|
| [port_00_LinkPort.md](port_00_LinkPort.md) | Port 00h (link port lines, link assist) |
| [ram_8674_header.md](ram_8674_header.md) | RAM 8674h: link header scratch (23 bytes, ioData=867D, ioNewData=8689) |
| [usb_84plus.md](usb_84plus.md) | 84+ USB hardware/software overview |

### Hooks

| File | Hook | Pointer block |
|---|---|---|
| [hooks_overview.md](hooks_overview.md) | OS:Hooks calling-convention overview (the `.db 83h` contract, install/chain) | : |
| [hooks_index.md](hooks_index.md) | Full WikiTI hook category index (26 entries, links + which we mirror) | : |
| [hook_9B78_LinkActivity.md](hook_9B78_LinkActivity.md) | Link Activity (fires on link-line edge, output ignored, suppressed during GetKey) | 9B78h |
| [hook_9B84_RawKey.md](hook_9B84_RawKey.md) | Raw Key (post-accept GetKey hook) | 9B84h |
| [hook_9B88_GetCSC.md](hook_9B88_GetCSC.md) | GetCSC (keypad-scan hook, fires inside `_GetKey`) | 9B88h |
| [hook_9B8C_Homescreen.md](hook_9B8C_Homescreen.md) | Homescreen (display/keypress/expr/context : the ambient-UI hook) | 9B8Ch |
| [hook_9BD0_SilentLink.md](hook_9BD0_SilentLink.md) | Silent Link (abort-silent-link, NOT receive : opposite of dead-ends memory's hope) | 9BD0h |
| [hook_9BD4_USBActivity.md](hook_9BD4_USBActivity.md) | USB Activity (interrupt-time USB hook, OS 2.30+) | 9BD4h |

## Pages we tried that returned 404

- `83Plus:Link_Protocol`
- `Link_Protocol`
- `83Plus:Protocols:Link`
- `Link_Guide`
- `83Plus:OS:Linking`
- `83Plus:Hooks:9BAD` (silent link hook entry, if it exists, lives at a different name)
- `83Plus:BCALLs:4EFA` (`_Rec1stByte` is in our include but undocumented on WikiTI)

If a high-level "DBUS protocol overview" page exists on WikiTI it didn't
respond to the obvious URL guesses. The vendored TI Link Guide at
`references/linkguide/` covers that ground.

## Notes for calc-master REQ work

The bcall calling-convention story is now clear enough to write code:

- **Header layout (RAM 8674h, 23 bytes):** byte 0 = machine ID, byte 1 = command, bytes 2/3 = size word (little-endian), byte 4+ = checksum scratch. `ioData` at 867D, `ioNewData` at 8689.
- **`iy+1Bh` setup:** every link bcall demands this; figure out the bit layout from `ti83plus.inc` or the linkguide PDF before writing asm.
- **Send a REQ:** populate header with target machine ID + REQ command (09h), put var type/name at ioData, call `_SendPacket` (4ED6). Awaits ACK.
- **Receive VAR header reply:** `_Get4Bytes` (4EF4) reads first 4 bytes into header; then `_GetSmallPacket` (4EEB) reads the rest into ioData (NOT the first 4 bytes : it specifically skips them).
- **Then send CTS, receive DATA via `_GetDataPacket` (4EEE) which DOES read first 4 bytes.**
- **`_SendVarCmd` (4A14) gotcha:** refuses to talk to machine IDs 82h/83h/73h. Our Pico must present a different ID when acting as the slave responding to a calc-master REQ.
- **`_LinkStatus` (50E3)** needs OS >=1.13.

Cross-reference these notes against the vendored linkguide before committing
any of them to asm.
