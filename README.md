# JTDSP16

Verilog core compatible with ATT WE DSP16, famous for being the heart of CAPCOM Q-Sound games. Unless explicitely stated, the design follows the official documentation from ATT.

Designed by Jose Tejada (aka jotego).

You can show your appreciation through
* [Patreon](https://patreon.com/topapate), by supporting releases
* [Paypal](https://paypal.me/topapate), with a donation

You can contact the author via Twitter at @topapate.

# Macros

Macro              | Effect
-------------------|---------------
SIMULATION         | Avoids some initial X's in sims
JTDSP16_FWLOAD     | Firmware load from files (see below)
JTDSP16_DEBUG      | Output ports with internal signals are available at the top level

# Supported Instructions

All the instructions needed by QSound firmware are supported.

The instructions not used by QSound are listed below.

  T    |   Operation      | Remarks
-------|------------------|------------
00101  | Z:aT     F1      | Unsupported
01101  | Z:R              | Unsupported
10010  | ifc CON  F2      | Unsupported
10101  | Z:y      F1      | Used by firmware but only the *rNzp case
11101  | Z:y x=X  F1      | Unsupported
110101 | icall            | Unsupported

Long immediate loads in the cache are not tested in the random tests, but they are supported.

# ROM load during simulation

ROM can be loaded using the ports for that purpose or if the **JTDSP16_FWLOAD** macro is declared, the ROM will be loaded from two hexadecimal files.

File             | Contents
-----------------|---------------------
dsp16fw_msb.hex  | MSB part of the ROM
dsp16fw_lsb.hex  | LSB part of the ROM

The files must be in the simulation directory.

# Instruction Details

Nemonic            |  T    | Cacheable   | Interruptable | Cycles
-------------------|-------|-------------|---------------|---------
goto JA            | 0/1   |      No     |         No    |   2
R=M (short)        | 2/3   | Yes         |   Yes         |   1
F1 Y = a1[l]       | 4     | Yes         |   Yes         |   2
F1 Z:aT[l]         | 5     | Yes         |   Yes         |   2
F1 Y               | 6     | Yes         |   Yes         |   1
F1 aT[l]=Y         | 7     | Yes         |   Yes         |   1
at=R               | 8     | Yes         |   Yes         |   2
R=aS               | 9/11  | Yes         |   Yes         |   2
R=N (long)         | 10    |       No    |   Yes         |   2
Y=R                | 12    | Yes         |   Yes         |   2
Z:R                | 13    | Yes         |   Yes         |   2
do/redo            | 14    |       No    |         No    |   1
R=Y                | 15    | Yes         |   Yes         |   2
call JA            | 16/17 |       No    |         No    |   2
ifc CON F2         | 18    | Yes         |   Yes         |   1
if  CON F2         | 19    | Yes         |   Yes         |   1
F1 Y=y[l]          | 20    | Yes         |   Yes         |   2
F1 Z:y[l]          | 21    | Yes         |   Yes         |   2
F1 x=Y             | 22    | Yes         |   Yes         |   1
F1 y[l]=Y          | 23    | Yes         |   Yes         |   1
goto B             | 24    |       No    |         No    |   2
F1 y=a0 x=*pt++[i] | 25    | Yes         |   Yes         |   2 / 1 in cache
if CON goto        | 26    |       No    |         No    |   1+2
icall              | 26*   |       No    |         No    |   3
F1 y=a1 x=*pt++[i] | 27    | Yes         |   Yes         |   2 / 1 in cache
F1 Y = a0[l]       | 28    | Yes         |   Yes         |   2
F1 Z:y  x=*pt++[i] | 29    | Yes         |   Yes         |   2
Reserved           | 30    |             |   Yes         |
F1 y=Y  x=*pt++[i] | 31    | Yes         |   Yes         |   2 / 1 in cache

# Document Ambiguities and Contradictions

1. Post increment for YAAU is said to be circular when re!=0, the register is equal to re and the post
increment is 1. But it is not clear whether it also applies when j=1 and \*r++j is used.

# The External Memory

External memory cannot be used to execute a program. It can only be used to access data via the pt register.

Memory   |  Access by registers  | Can be loaded into
---------|-----------------------|-------------------
RAM      | r0,r1,r2,r3           | a0, a1, y, any register
ROM      | pc, pt                | x
external | pt                    | x

Originally in DSP16A, AB pins were 3 stated. Because AB in this implementation can only be used with the pt register, bits AB[15:12] are low when the external memory is not accessed. When pt is used to access the external memory AB[15:12] will not be zero. So peeking AB[15:12] is a way of determining that an external access has occured.

# The Cache

The cache does not accept instructions that alter the program flow or that take two memory words (i.e. the long immediate instruction).

The cache cannot be used on external memory. This might be different on original hardware.

Loops in cache is tricky because of:

* Double cycle instructions may affect the loop control
* Single instruction loops are an exception (NI=1)
* Output PC value may be altered when the instruction before the loop start takes two cycles

# Clock Timing

The original design probably divides down the clock to implement some kind of dynamic logic. I keep the divided clock design and I use it in this way:

1. Memories are allowed to operate at full clock speed. This makes it possible to latch inputs and outputs and eases routing and instantiation as BRAM in FPGA

2. The DAU uses the two phases of the divided clock to latch intermmediate operations; easing timing constraints.


# Resource Usage

This is a comparison done for MiST (Altera V).

Module  | Logic cells | Registers | M9K
--------|-------------|-----------|-------
JTDSP16 |   2471      |   612     | 12
Z80     |   2450      |   357     |  0
JT51    |   3572      |   1652    | 12


# Related Projects

Other sound chips from the same author

Chip                   | Repository
-----------------------|------------
YM2203, YM2612, YM2610 | [JT12](https://github.com/jotego/jt12)
YM2151                 | [JT51](https://github.com/jotego/jt51)
YM3526                 | [JTOPL](https://github.com/jotego/jtopl)
YM2149                 | [JT49](https://github.com/jotego/jt49)
sn76489an              | [JT89](https://github.com/jotego/jt89)
OKI 6295               | [JT6295](https://github.com/jotego/jt6295)
OKI MSM5205            | [JT5205](https://github.com/jotego/jt5205)