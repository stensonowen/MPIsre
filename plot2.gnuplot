#!/usr/bin/gnuplot

set datafile separator ","
set xrange [1:4]
set xtics 1
set yrange [0:200]
set xlabel 'Processes'
set ylabel 'Time (seconds)'
set title 'Seconds to find all primes less than 100,000,000 on N cores'
set grid
plot 'data2.csv' using 1:2 title 'Run 1', \
     'data2.csv' using 1:3 title 'Run 2', \
     'data2.csv' using 1:4 title 'Run 3', \
     'data2.csv' using 1:5 smooth cspline title 'Average' lc rgb '#000000'

