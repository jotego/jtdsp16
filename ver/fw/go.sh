#!/bin/bash

verilator ../../hdl/*.v --cc --top-module jtdsp16 --exe test.cc vcd.cc rtl.cc mametrace.cc WaveWritter.cc \
    --trace -DJTDSP16_DEBUG -DJTDSP16_DUMP || exit $?
make -j -C obj_dir -f Vjtdsp16.mk Vjtdsp16 || exit $?

sim $* -vcd >(vcd2fst -v - -f test.fst)