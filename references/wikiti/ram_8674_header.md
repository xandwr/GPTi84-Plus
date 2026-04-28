# 83Plus:RAM:8674 - Link Header

Source: https://wikiti.brandonw.net/index.php?title=83Plus:RAM:8674

## Overview

- **Official Name:** header
- **Memory Address:** 8674h
- **Length:** 23 bytes

## Purpose

Scratch space for sending/receiving packets in connection with link-related BCALLs.

## Notable Subregions

Two named locations fall within this memory area:

- **ioData** at 867Dh : "typical location in certain packets to find important data"
- **ioNewData** at 8689h : same role, different packet contexts

## Cross-references (from include / other bcall pages)

- header+0: machine ID
- header+1: command byte
- header+2,+3: size word (also reused by `_Send4BytePacket` / `_SendAck` for the 2 bytes after machine ID + command)
- header+4: running checksum byte (zero before first `_SendDataByte` call)

Last updated February 22, 2007.
