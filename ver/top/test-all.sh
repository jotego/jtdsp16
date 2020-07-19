#!/bin/bash

FAILS=

for i in tests/*.asm; do
    go.sh $i --quiet $* || FAILS="$FAILS $i"
done

if [ ! -z "$FAILS" ]; then
    figlet FAIL
    echo "Failed tests:"
    echo $FAILS
else
    figlet PASS
fi
