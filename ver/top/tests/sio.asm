# write a value to the serial register
pdx0=10
srta=0x10
sdx=0xaa05

# wait a bit for the transfer to start
c1=0x60
loop:
a0=pioc
if c1ge goto loop

# write another value to serial
# the serial output should not stop
# because of the double buffer
sdx=0xf781

end:
goto end
