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

r0 = 0x00e3
r1 = 0x0314
*r0 = r1    # set main offset = 0x0314 update_loop_1
sioc = 0x02e8   # setup Serial I/O
pioc = 0x3800   # setup Parallel I/O, disable INT interrupts
y = 0x0120      # pan 0x120 = centre (PanTable_FrontL + 0x010)
r0 = 0x0080    # 0080: pan position
# set pan position of 19 channels (19??)
do 19 {
    *r0++ = y
}

end:
pdx0=0xdead     # finish
goto end
