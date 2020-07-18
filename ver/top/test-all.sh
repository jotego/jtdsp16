#!/bin/bash

for i in tests/*.asm; do
    go.sh $i
done
