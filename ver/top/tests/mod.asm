# Test modular increment
r0 = 0x1000
rb = 0x1000
re = 0x1003
*r0++=r0
*r0++=r0
*r0++=r0
*r0++=r0
*r0++=r0
*r0++=r0
*r0++=r0
*r0++=r0
*r0++=r0
*r0++=r0
end:
goto end
