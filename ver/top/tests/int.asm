goto start
a0=a0+y
ireturn
a0=a1+y

start:
y=1
pdx0=0xcafe
pioc=0x20
pdx0=0xbeef
pdx0=0xcafe
pdx0=0xbabe
pioc=0x0
end:
goto end
