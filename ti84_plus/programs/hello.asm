; hello.asm: minimal "HELLO WORLD" assembly program for the TI-84 Plus.
; Assembled with spasm-ng, produces an .8Xp that runs via Asm(prgmHELLO).
;
; No TI SDK headers required: bcall vectors and constants are spelled out
; here so this file builds against a stock spasm-ng install.

#define bcall(x) rst 28h \ .dw x

_HomeUp   .equ 4558h
_PutS     .equ 450Ah
_NewLine  .equ 452Eh
_GetKey   .equ 4972h

; .8Xp programs load at $9D93 with a two-byte AsmPrgm marker.
.org    9D93h
.db     0BBh, 6Dh

    bcall(_HomeUp)
    ld      hl, msg
    bcall(_PutS)
    bcall(_NewLine)
    bcall(_GetKey)
    ret

msg:
    .db     "HELLO WORLD", 0

.end
