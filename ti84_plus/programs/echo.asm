; echo.asm: receive a byte over the link port and re-transmit it.
; Loops until ON is pressed. Used to validate Pico DBUS bit/byte encoding
; against a real calc, decoupled from the OS packet protocol.
;
; Assemble: spasm echo.asm ECHO.8xp
; Run on calc: Asm(prgmECHO)
;
; bcall vectors and constants are spelled out so this builds against a
; stock spasm-ng install with no SDK headers.

#define bcall(x) rst 28h \ .dw x

_HomeUp      .equ 4558h
_PutS        .equ 450Ah
_NewLine     .equ 452Eh
_Rec1stByte  .equ 4EFAh    ; out: A=byte, CF set on timeout/error
_SendAByte   .equ 4EE5h    ; in:  A=byte, CF set on error
_GetCSC      .equ 4018h    ; out: A=keycode (0 if none)

skOn         .equ 37h      ; _GetCSC scan code for ON

.org    9D93h
.db     0BBh, 6Dh

    bcall(_HomeUp)
    ld      hl, banner
    bcall(_PutS)
    bcall(_NewLine)

main_loop:
    bcall(_GetCSC)
    cp      skOn
    ret     z              ; exit on ON

    bcall(_Rec1stByte)
    jr      c, main_loop   ; timeout/error: try again (also re-checks ON)

    bcall(_SendAByte)      ; A still holds the received byte
    jr      main_loop      ; ignore CF; if TX failed the host will notice

banner:
    .db     "ECHO LINK", 0

.end
