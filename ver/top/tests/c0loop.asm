c0=0xf6
y=1

loop:
a0=a0+y
if c0lt goto loop

c1=0x70
loop2:
a1=a1+y
if c1ge goto loop2

end:
goto end
