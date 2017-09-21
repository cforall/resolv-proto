# Plots output from resolv-proto benchmarks
# requires parameters bu, co, td

set terminal pdfcairo size 12cm,10cm
set datafile separator ","

set key outside

set output 'plots/runtime.pdf'
set xtics rotate
set ylabel 'time(s)'
#set yrange [0:120]
set yrange [0:25]

plot bu using ($2+$3):xticlabels(1) with points title 'BU', \
     co using ($2+$3):xticlabels(1) with points title 'CO', \
     td using ($2+$3):xticlabels(1) with points title 'TD'

set output 'plots/mem.pdf'
set ylabel 'peak memory(KB)'
set yrange [0:*]

plot bu using 5:xticlabels(1) with points title 'BU', \
     co using 5:xticlabels(1) with points title 'CO', \
     td using 5:xticlabels(1) with points title 'TD'
