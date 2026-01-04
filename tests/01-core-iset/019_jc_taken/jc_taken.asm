org 0x1000

stc                 ; CF = 1
jc taken
mov al, 0x00        ; should be skipped
hlt

taken:
mov al, 0x01
hlt