bits 16
org 0x1000

    mov ax, 1
    cmp ax, 1
    jnz .bad
    mov ax, 0x4444
    hlt

.bad:
    mov ax, 0x9999
    hlt