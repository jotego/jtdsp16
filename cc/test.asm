# Test register loads
move r0 = 0xFFFF
move r0 = 0
move r1 = 0x11
move r2 = 0x22
move r3 = 0x33
move r1 = 0x1111
move r2 = 0x2222
move r3 = 0x3333

# Test RAM writes
move r0 = 0xBE
*r0++=r0
*r0++=r1
*r0++=r2
*r0++=r3
move r0 = 0



# End