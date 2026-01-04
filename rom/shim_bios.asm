bits 16

; Build a 64KB ROM mapped at physical 0xF0000..0xFFFFF
; Reset vector is at physical 0xFFFF0 == offset 0xFFF0 inside this ROM.

org 0x0000
bios_entry:
    cli
.hang:
    hlt
    jmp .hang

; Pad until the reset vector location within the ROM image
times 0xFFF0 - ($ - $$) db 0

reset_vector:
    ; Far jump to F000:0000 (start of this ROM segment)
    db 0xEA
    dw 0x0000
    dw 0xF000