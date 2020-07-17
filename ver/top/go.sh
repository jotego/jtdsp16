#!/bin/bash


function add_dir {
    if [ ! -d "$1" ]; then
        echo "ERROR: add_dir (sim.sh) failed because $1 is not a directory"
        exit 1
    fi
    processF=no
    echo "Adding dir $1 $2" >&2
    for i in $(cat $1/$2); do
        if [ "$i" = "-sv" ]; then 
            # ignore statements that iVerilog cannot understand
            continue; 
        fi
        if [ "$processF" = yes ]; then
            processF=no
            # echo $(dirname $i) >&2
            # echo $(basename $i) >&2
            dn=$(dirname $i)
            if [ "$dn" = . ]; then
                dn=$1
            fi
            add_dir $dn $(basename $i)
            continue
        fi
        if [[ "$i" = -F || "$i" == -f ]]; then
            processF=yes
            continue
        fi
        fn="$1/$i"
        if [ ! -e "$fn" ]; then
            (>&2 echo "Cannot find file $fn")
        fi
        echo $fn
    done
}


iverilog test.v $(add_dir ../../hdl jtdsp16.f) -o sim -s test && sim -lxt
