bits 16
org 0x1000

    mov sp, 0x2000
    mov ax, 0xBEEF
    push ax
    mov ax, 0x0000
    pop ax
    hlt