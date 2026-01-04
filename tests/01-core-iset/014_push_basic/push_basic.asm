; push_basic.asm
bits 16
org 0x0000

start:
    ; establish a known stack
    xor ax, ax
    mov ss, ax
    mov sp, 0x2000

    ; load a known value and push it
    mov ax, 0xBEEF
    push ax

    hlt