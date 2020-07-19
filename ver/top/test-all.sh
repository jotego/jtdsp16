#!/bin/bash

FAILS=

for i in tests/*.asm; do
    go.sh $i --quiet $* || FAILS="$FAILS $i"
done

if [ ! -z "$FAILS" ]; then
    echo "Failed tests:"
    echo $FAILS
fi
