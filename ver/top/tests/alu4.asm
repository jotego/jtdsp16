y=0xa
r0=0
*r0++=y
y=0xb
*r0++=y
y=0xc
*r0++=y
y=0xd
*r0=y
y=0
# Sum everything now
r0=0
y=*r0++ # a
a0=a0+y
y=*r0++ # b
a0=a0+y
y=*r0++ # c
a0=a0+y
y=*r0++ # d
a0=a0+y
end:
goto end
