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
memlim=""
repeat=1
reading="T"
dryrun=""
for a in "$@"; do
    case $a in
    -m|--mode) reading="M" ;;
    -t|--test) reading="T" ;;
    -f|--file) reading="F" ;;
    -r|--repeat) reading="R" ;;
    -l|--mem-limit)
        reading="L"
        if [ ! -x ../../timeout/timeout ]; then
            echo missing ../../timeout/timeout
            exit 1
        fi
        # default to 8GB
        memlim=8388608
        ;;
    -a|--all-tests)
        for f in *.in; do
            tests+=(`basename $f .in`)
        done
        ;;
    -d|--dry-run) dryrun="y" ;;
    *)
        case $reading in
        T) tests+=($a) ;;
        M) modes+=($a) ;;
        F)
            if [ ! -f $a ]; then
                echo "Invalid test suite $a"
                exit 1
            fi
            mapfile -t -O ${#tests[@]} tests < $a
            ;;
        R) repeat=$a ;;
        L) memlim=$a ;;
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
    if [ $dryrun ]; then
        echo "no tests; autogenerating list"
    fi
    for f in *.in; do
        tests+=(`basename $f .in`)
    done
fi

if [ $dryrun ]; then
    for t in "${tests[@]}"; do
        echo $t
    done
    exit 0
fi

for m in "${modes[@]}"; do
    # print header for mode
    printf "\n== $m ==\n"
    c="../rp-$m -q"
    if [ $memlim ]; then
        outfile="$outdir/$m-tests.txt"
		if [ -f $outfile ]; then
			truncate -s 0 $outfile
		fi
        for t in "${tests[@]}"; do
            printf "%24s\t" $t
            if ../../timeout/timeout -m=$memlim --no-info-on-success -c $c $t.in; then
                echo $t >> $outfile
                printf "SUCCESS\n"
            fi
        done
    else
        outfile="$outdir/$vercode-$m.csv"
        echo -n '"test"' | tee $outfile
        for (( i=1; i<=$repeat; i++ )); do
            echo -n ',"user(s)","sys(s)","wall(s)","max-mem(KB)"' | tee -a $outfile
        done
        echo | tee -a $outfile
        for t in "${tests[@]}"; do
            printf "\"%s\"" $t | tee -a $outfile
            for (( i=1; i<=$repeat; i++ )); do
                /usr/bin/time -f ",%U,%S,%e,%M" $c $t.in 2>&1 | tr -d '\n' | tee -a $outfile
            done
            echo | tee -a $outfile
        done
    fi
done
