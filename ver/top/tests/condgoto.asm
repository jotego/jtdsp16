#Conditional goto
r0=0xf00
r1=0xf00
y=10
x=1
a0=y
a1=y
a1=a0-y    # EQ flag
if eq goto check1
r0=0xdead

check1:
a1=a0+y    # NEQ flag
if ne goto check2
r1=0xdead

check2:
# now check false conditions
r2=0xbad
a0=p
a0=a0+y
bad_end:
if eq goto bad_end

loop:
r2=0xcafe
goto loop

