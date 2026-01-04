; Smoke test SUB instruction
org 0x100  ; COM-style binary start

start:
    mov ax, 5
    sub ax, 1          ; AX = 4
    ; store result to memory to inspect
    mov [0x200], ax

    mov bx, 3
    sub ax, ax         ; AX = 0
    mov [0x202], ax

    mov dx, 0xFFFF
    sub dx, bx         ; DX = 0xFFFC
    mov [0x204], dx

    mov cx, 1
    sub cx, dx         ; CX = 2 (wraparound)
    mov [0x206], cx

    hlt                ; stop CPU