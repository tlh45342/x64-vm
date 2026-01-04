bits 16
org 0x0000

%define HANDLER30_OFF 0x1100
%define HANDLER31_OFF 0x1120

; Vector 0x30 @ 0x00C0, vector 0x31 @ 0x00C4
times 0x00C0 db 0
dw HANDLER30_OFF
dw 0x0000
dw HANDLER31_OFF
dw 0x0000

; pad up to 0x1000 where we start executing
times 0x1000-($-$$) db 0

start:
    mov bx, 0x0000
    mov cx, 0x0000
    int 0x30
    int 0x31
    hlt

; handler 0x30 at 0x1100
times HANDLER30_OFF-($-$$) db 0
handler30:
    mov bx, 0x1111
    iret

; handler 0x31 at 0x1120
times HANDLER31_OFF-($-$$) db 0
handler31:
    mov cx, 0x2222
    iret