y=1
r0=1
do 4 {
    a0=a0+y
    a1=a1+y
    move *r0++=a0
}

x=*r0--
x=*r0--
y=*r0--

end:
goto end


