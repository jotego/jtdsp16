r0=1
r2=2
r3=3
call change

loop:
rb=0xcafe
goto loop

change:
a0=pr
a1=pi
r0=4
r2=5
r3=6
return
