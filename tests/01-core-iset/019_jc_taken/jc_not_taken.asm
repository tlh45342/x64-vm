org 0x1000

clc                 ; CF = 0
jc taken
mov al, 0x02
hlt

taken:
mov al, 0x03
hlt