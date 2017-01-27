#!/bin/bash

cd `dirname "${BASH_SOURCE[0]}"`

tests=()
if [ $# -eq 0 ]; then
    # get tests from all .in files in test directory if none given
    for f in *.in; do
        tests+=(`basename $f .in`)
    done
else
    # take test names from arguments
    tests=$@
fi

for t in "${tests[@]}"; do
    printf "  %-16s" $t
    ../rp $t.{in,test}
    if [ -e $t.out ]; then
        diff $t.{out,test} > $t.diff
        if [ $? -eq 0 ]; then
            printf "  PASSED\n"
            rm $t.{test,diff}
        else
            printf "  FAILED\n"
            cat $t.diff
        fi
    else
        printf "  NEW\n"
        mv $t.{test,out}
    fi
done
