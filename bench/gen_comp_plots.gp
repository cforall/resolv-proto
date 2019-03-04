# Copyright (c) 2015 University of Waterloo
#
# The contents of this file are covered under the licence agreement in 
# the file "LICENCE" distributed with this repository.

# Plots output from resolv-proto benchmarks
# requires parameters new, old, mode

set terminal pdfcairo size 10cm,10cm
set datafile separator ","

set output 'plots/'.mode.'-comp-runtime.pdf'
set xtics rotate
set ylabel 'time(s)'

plot old using ($2+$3):xticlabels(1) with points title 'Old', \
     new using ($2+$3):xticlabels(1) with points title 'New'

set output 'plots/'.mode.'-comp-mem.pdf'
set ylabel 'peak memory(KB)'

plot old using 5:xticlabels(1) with points title 'Old', \
     new using 5:xticlabels(1) with points title 'New'
