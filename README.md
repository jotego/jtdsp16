# jtdsp16

Verilog core compatible with ATT WE DSP16, famous for being the heart of CAPCOM Q-Sound games

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

# Clock Divider

Original chip runned at 60MHz clock with an internal clock divider by 2. In this design nothing needs to run at full speed so the internal clock divider is by-passed by default. It can be enabled by declaring the macro:

**JTDSP16_DIV**

This causes currently problems with the memories and cannot be used.

