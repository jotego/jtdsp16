#!/bin/bash

verilator ../../hdl/*.v --cc --top-module jtdsp16 --exe test.cc --trace || exit $?
make -j -C obj_dir -f Vjtdsp16.mk Vjtdsp16 || exit $?
sim $*
