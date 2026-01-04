; int10_hello.asm
bits 16
org 0x0000

start:
    cld
    mov si, msg

.print:
    lodsb
    test al, al
    jz .done

    mov ah, 0x0E        ; teletype output
    mov bh, 0x00        ; page
    mov bl, 0x0F        ; attribute (white)
    int 0x10

    jmp .print

.done:
    hlt

msg db "HELLO", 13, 10, 0