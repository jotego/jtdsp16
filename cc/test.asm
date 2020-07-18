# Test register loads
move r0 = 0xFFFF
move r0 = 0
move r1 = 0x11
move r2 = 0x22
move r3 = 0x33
move r1 = 0x1111
move r2 = 0x2222
move r3 = 0x3333

# j, k, rb, re
move j = 0x4004
move k = 0x5005
move rb= 0x6006
move re= 0x7007

# Test ++ and --
*r0++=r0
*r0--=r1
*r0++=r2
*r0--=r3

# Test ++j on r2
*r2++j=r3
*r2++j=r3
*r2++j=r3
*r2++j=r3

# Test no post-inc
*r0=r3
*r1=r3

# Test modular increment
move r0 = 0x1000
move rb = 0x1000
move re = 0x1003
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

# Test store and loads from RAM
r0=10
r1=1
r2=2
r3=3
*r0++=r1
*r0++=r2
*r0=r3
r1=*r0--
r2=*r0--
r3=*r0


move r0 = 0xdead
# End