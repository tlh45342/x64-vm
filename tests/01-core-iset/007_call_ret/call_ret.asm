bits 16
org 0x1000

    mov ax, 0x0000
    call func
    hlt

func:
    mov ax, 0x3333
    ret