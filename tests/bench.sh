#!/bin/bash

cd `dirname "${BASH_SOURCE[0]}"`

outdir="../bench"
if [ ! -d $outdir ]; then
    mkdir $outdir
fi
outname="$outdir/test-`date +%y%m%d%H%M`-`git log -1 --format=%h`"

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

outfile="${outname}.csv"
echo '"test","user(s)","sys(s)","wall(s)","max-mem(KB)"' | tee $outfile

for t in "${tests[@]}"; do
    printf "\"%s\"" $t | tee -a $outfile
    /usr/bin/time -f ",%U,%S,%e,%M" ../rp -q $t.in 2>&1 | tee -a $outfile
done
