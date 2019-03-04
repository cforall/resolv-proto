#!/bin/bash

# Copyright (c) 2015 University of Waterloo
#
# The contents of this file are covered under the licence agreement in 
# the file "LICENCE" distributed with this repository.

cd `dirname "${BASH_SOURCE[0]}"`

outdir="./out"
if [ ! -d $outdir ]; then
    mkdir $outdir
fi

# timestamped/git versioned identifier for this benchmark run
vercode="`date +%y%m%d%H%M`-`git log -1 --format=%h`"

# read testing arguments; by default, reads tests off argument list
# can take all tests from folder by passing --all-tests argument
# can specify which resolver modes to run after the --mode flag
# can explicitly specify a list of tests after the --test flag
tests=()
modes=()
reading="T"
for a in "$@"; do
    case $a in
    -m|--mode) reading="M" ;;
    -t|--test) reading="T" ;;
    -a|--all-tests)
        for f in *.in; do
            tests+=(`basename $f .in`)
        done
        ;;
    *)
        case $reading in
        T) tests+=($a) ;;
        M) modes+=($a) ;;
        esac
    ;;
    esac
done

# if no modes/tests set, set defaults (all modes currently compiled / all tests, skipping non-easy)
if [ ${#modes[@]} -eq 0 ]; then
    for m in ../rp-*; do
        modes+=(${m#../rp-}) # trim prefix from mode
    done
fi
if [ ${#tests[@]} -eq 0 ]; then
    for f in *.in; do
        t=`basename $f .in`
        if [ ! -f $t-easy.in ]; then  # skip tests that needed to be nerfed
            tests+=($t)
        fi
    done
fi

for m in "${modes[@]}"; do
    printf "\n== $m ==\n"
    outfile="$outdir/$vercode-$m.csv"
    echo '"test","user(s)","sys(s)","wall(s)","max-mem(KB)"' | tee $outfile
    for t in "${tests[@]}"; do
        printf "\"%s\"" $t | tee -a $outfile
        /usr/bin/time -f ",%U,%S,%e,%M" ../rp-$m -q $t.in 2>&1 | tee -a $outfile
    done
done
