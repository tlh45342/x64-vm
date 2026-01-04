; Assemble: nasm -f bin mov_add.asm -o mov_add.bin
bits 16
org 0x1000

    mov ax, 0x1234
    add ax, 0x0001
    hlt