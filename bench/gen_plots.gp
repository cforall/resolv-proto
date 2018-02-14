# Plots output from resolv-proto benchmarks
# requires parameters:
#        n: number of data files
#    files: data files
#   labels: labels for data files

set terminal pdfcairo size 12cm,10cm
set datafile separator ","

set key outside

set output 'plots/runtime.pdf'
set xtics rotate
set ylabel 'time(s)'
#set yrange [0:120]
set yrange [0:*]

plot for [i=1:n] word(files,i) using ($2+$3):xticlabels(1) with points title word(labels,i)

set output 'plots/mem.pdf'
set ylabel 'peak memory(KB)'
#set yrange [0:*]

plot for [i=1:n] word(files,i) using 5:xticlabels(1) with points title word(labels,i)
