#!/bin/bash

FAILS=

for i in tests/*.asm; do
    i=$(basename $i)
    noasm=$(basename $i .asm)
    go.sh $i --quiet $* --nodump > tests/${noasm}.log
    if [ $? = 0 ]; then
        printf "%-12s PASS\n" $noasm
    else
        printf "%-12s FAIL\n" $noasm
        FAILS=$(printf "$FAILS\n$i")
    fi
done

if [ ! -z "$FAILS" ]; then
    figlet FAIL
    echo "Failed tests:"
    echo $FAILS
else
    figlet PASS
fi
