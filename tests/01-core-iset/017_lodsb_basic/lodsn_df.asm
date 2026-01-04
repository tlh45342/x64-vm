org 0x1000

std                 ; DF = 1
mov si, msg+1
lodsb               ; AL = 'B'
lodsb               ; AL = 'A'
hlt

msg db 'A', 'B'