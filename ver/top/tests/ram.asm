r0=0x100
r1=0x23
r2=0xbabe
r3=0xa055
j=3
# Test ++ and --
*r0++=r1
*r0++=r2
*r0=r3
r1=*r0--
r2=*r0--
r3=*r0--
end:
goto end
