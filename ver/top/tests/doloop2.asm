pdx0=0xdead   # turns off automatic simulation end

pioc = 0x3800 # disable INT interrupts
y = 0x0000
r0 = 0x000
do 127 {
    *r0++ = y
}

redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 127
redo 16

end:
pdx0=0xdead     # finish
goto end
