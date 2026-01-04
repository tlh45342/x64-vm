bits 16
org 0x1000

    mov dx, 0x03F8      ; COM1 data port
    mov ax, 'A'         ; AL = 'A'
    out dx, al
    hlt