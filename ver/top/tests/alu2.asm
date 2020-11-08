x=0x20
y=0x30
a0=p,p=x*y  # p=0x600
a1=a0+p  # a1 = 0x600
a1=a1+p  # a1 = 0xC00
y=0
yl=1     # y = 1
a0=a1-y  # a0 = 0xBFF
end:
goto end
