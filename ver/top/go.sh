#!/bin/bash


function add_dir {
    if [ ! -d "$1" ]; then
        echo "ERROR: add_dir (sim.sh) failed because $1 is not a directory"
        exit 1
    fi
    processF=no
    # echo "Adding dir $1 $2" >&2
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

# Update assembler tool
pushd . > /dev/null
cd ../../cc
make dsp16as || exit $?
popd  > /dev/null

TESTNAME=tests/rloads.asm
TESTSET=0
CLOBBER=0
QUIET=0
MACROS=
NODUMP=0

while [ $# -gt 0 ]; do
    case "$1" in
        -h)
            cat <<EOF
    Usage: go.sh path-to-asm file
EOF
            exit 0;;
        -q | --quiet)
            QUIET=1;;
        -c | --clobber)
            CLOBBER=1;;
        --nodump)
            MACROS="$MACROS -DNODUMP"
            NODUMP=1;;
        *)
            if [ $TESTSET != 0 ]; then
                echo "Test name had already been set to " $TESTNAME
                exit 1
            else
                if [ -e $1 ]; then
                    TESTNAME=$1
                else
                    TESTNAME=tests/$1
                    TESTNAME=${TESTNAME%.asm}.asm
                fi
                TESTSET=1
            fi;;
    esac
    shift
done
echo $TESTNAME
../../cc/dsp16as $TESTNAME || exit $?
CHECKFILE=${TESTNAME%.asm}.out

if [ ! -e $CHECKFILE ]; then
    echo "Warning: check file for simulation " $CHECKFILE " not found"
fi

iverilog test.v $(add_dir ../../hdl jtdsp16.f) -o sim \
    -s test -DSIMULATION $MACROS || exit $?

if [ $QUIET = 0 ]; then
    sim -lxt | tee log.out
else
    sim -lxt > log.out
fi

# Deletes the LXT info line
if [ $NODUMP = 0 ]; then
    sed -i 1d log.out
fi
sed -i /pc=/d\;/pi=/d *.out

function clobber {
    if [ $CLOBBER = 1 ]; then
        mv -v log.out $CHECKFILE
        git add $CHECKFILE $TESTNAME
    fi
}

if [ -e $CHECKFILE ]; then
    if diff --brief log.out $CHECKFILE > /dev/null ; then
        if [ $QUIET = 1 ]; then
            echo PASS
        else
            figlet PASS
        fi
    else
        if [ $CLOBBER = 1 ]; then
            clobber
        else
            if [ $QUIET = 0 ]; then
                echo ---------- comparison --------------
                diff --side-by-side log.out $CHECKFILE
                figlet FAIL
            else
                echo FAIL
            fi
            echo "Use this to copy the current results file:"
            echo "mv log.out $CHECKFILE; git add $CHECKFILE $TESTNAME"
            exit 1
        fi
    fi
else
    clobber
    echo "No check file, use this to copy the current results file:"
    echo "mv log.out $CHECKFILE; git add $CHECKFILE $TESTNAME"
    exit 1
fi
