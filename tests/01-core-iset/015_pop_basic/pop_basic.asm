; pop_basic.asm
bits 16
org 0x0000

start:
    xor ax, ax
    mov ss, ax
    mov sp, 0x2000

    ; push a value, clobber AX, then pop into BX
    mov ax, 0x1234
    push ax

    mov ax, 0x0000
    pop bx

    hlt