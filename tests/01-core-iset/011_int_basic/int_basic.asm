bits 16
org 0x0000

; --- IVT setup in the image itself ---
; Vector 0x30 entry lives at 0x30*4 = 0x00C0
times 0x00C0 db 0
dw handler30          ; offset
dw 0x0000             ; segment

; pad up to 0x1000 where we start executing
times 0x1000-($-$$) db 0

start:
    mov ax, 0x1111
    int 0x30
    hlt

; place handler at 0x1100
times 0x1100-($-$$) db 0
handler30:
    mov ax, 0xBEEF
    iret