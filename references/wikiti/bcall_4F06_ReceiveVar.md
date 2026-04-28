# 83Plus:BCALLs:4F06

Source: https://wikiti.brandonw.net/index.php?title=83Plus:BCALLs:4F06

## Synopsis

**Unofficial Name:** ReceiveVar

**BCALL Address:** 4F06

"Receives a variable over the link port."

## Inputs

- (ioData): variable size
- ioData+2: variable type and name
- (ioNewData): bit 7 set to receive to archive
- (sndRecState): 15h
- 1,(iy+1Bh) must be reset
- iy+1Bh set up accordingly

## Outputs

None.

## Destroys

All.

## Comments

Throws ERR:LINK on problems. If battery power is low, sends a skip/exit
packet with code 12, receives acknowledgement, then jumps to JForceCmdNoChar.

The entry point is "designed to work right after receiving a variable header
packet," making it suitable for use with GetSmallPacket or similar functions.

**Note for our use case:** confirms the dead-end finding from
project_calc_to_pico_recv_dead_ends.md : `_ReceiveVar` is a passive
receive helper that runs *after* a variable header packet has already been
received. It is NOT a calc-master REQ initiator. Calc-master REQ must be
built from `_SendPacket` / `_Send4BytePacket`.
