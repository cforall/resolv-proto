# Plots output from resolv-proto benchmarks
# requires parameters td, bu

set terminal pdfcairo size 10cm,10cm
set datafile separator ","

set output 'plots/runtime.pdf'
set xtics rotate
set ylabel 'time(s)'

plot td using ($2+$3):xticlabels(1) with points title 'TD', \
     bu using ($2+$3):xticlabels(1) with points title 'BU'

set output 'plots/mem.pdf'
set ylabel 'peak memory(KB)'

plot td using 5:xticlabels(1) with points title 'TD', \
     bu using 5:xticlabels(1) with points title 'BU'
