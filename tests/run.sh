#!/bin/bash

source_dir=`dirname "${BASH_SOURCE[0]}"`

tests=()
if [ $# -eq 0 ]; then
    # get tests from all .in files in test directory if none given
    for f in $source_dir/*.in; do
        tests+=(`basename $f .in`)
    done
else
    # take test names from arguments
    tests=$@
fi

for t in "${tests[@]}"; do
    printf "  %-16s" $t
    $source_dir/../rp $source_dir/$t.{in,test}
    if [ -e $source_dir/$t.out ]; then
        diff $source_dir/$t.{out,test} > $source_dir/$t.diff
        if [ $? -eq 0 ]; then
            printf "  PASSED\n"
            rm $source_dir/$t.{test,diff}
        else
            printf "  FAILED\n"
            cat $source_dir/$t.diff
        fi
    else
        printf "  NEW\n"
        mv $source_dir/$t.{test,out}
    fi
done
