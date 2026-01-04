bits 16
org 0x1000

    mov ax, 0x1111
    jmp short .skip
    mov ax, 0x2222      ; should be skipped
.skip:
    hlt