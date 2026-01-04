org 0x1000

cld                 ; DF = 0
mov si, msg
lodsb               ; AL = 'A'
lodsb               ; AL = 'B'
hlt

msg db 'A', 'B'