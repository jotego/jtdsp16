#!/bin/bash

verilator ../../hdl/*.v --cc --top-module jtdsp16 --exe test.cc vcd.cc --trace -DJTDSP16_DEBUG || exit $?
make -j -C obj_dir -f Vjtdsp16.mk Vjtdsp16 || exit $?

sim $*

## | vcd2fst -v - -f test.fst