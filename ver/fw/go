#!/bin/bash

if [ ! -e $JTUTIL/model/dsp16/dsp16_model.h ]; then
    echo "You need model/dsp16/dsp16_model.h from the jtutil repository"
    exit 1
fi

verilator ../../hdl/*.v --cc --top-module jtdsp16 --exe \
    test.cc vcd.cc rtl.cc mametrace.cc WaveWritter.cc \
    $JTUTIL/model/dsp16/dsp16_model.c \
    --trace -DJTDSP16_DEBUG -DJTDSP16_DUMP || exit $?
export CPPFLAGS="$CPPFLAGS -O3 -I$JTUTIL/model/dsp16"
make -j -C obj_dir -f Vjtdsp16.mk Vjtdsp16 || exit $?

if which vcd2fst; then
    sim $* -vcd >(vcd2fst -v - -f test.fst)
else
    sim $* -vcd test.vcd
fi