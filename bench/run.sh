#!/bin/bash

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

# if no modes/tests set, set defaults
if [ ${#modes[@]} -eq 0 ]; then
    modes=(td bu)
fi
if [ ${#tests[@]} -eq 0 ]; then
    tests=(default-easy default2-easy tuple-return less-poly-easy less-poly2-easy more-poly-easy fewer-basic-easy fewer-overloads-easy fewer-overloads2 more-overloads-easy most-overloads-easy long-tail-overloads-easy fewer-parms more-parms-easier shallower-nesting deeper-nesting-easy fewer-decls more-decls-easy more-decls2-easy)
fi

for m in "${modes[@]}"; do
    printf "\n== $m ==\n"
    outfile="$outdir/$m-$vercode.csv"
    echo '"test","user(s)","sys(s)","wall(s)","max-mem(KB)"' | tee $outfile
    for t in "${tests[@]}"; do
        printf "\"%s\"" $t | tee -a $outfile
        /usr/bin/time -f ",%U,%S,%e,%M" ../rp-$m -q $t.in 2>&1 | tee -a $outfile
    done
done

#outname="$outdir/test-`date +%y%m%d%H%M`-`git log -1 --format=%h`"
# outfile="${outname}.csv"
# echo '"test","user(s)","sys(s)","wall(s)","max-mem(KB)"' | tee $outfile

# for t in "${tests[@]}"; do
#     printf "\"%s\"" $t | tee -a $outfile
#     /usr/bin/time -f ",%U,%S,%e,%M" ../rp -q $t.in 2>&1 | tee -a $outfile
# done
