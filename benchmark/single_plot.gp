set terminal pngcairo size 1200, 600 enhanced font 'Helveca, 22'
set output out_file
set datafile separator ","
set pointsize 1
set style data linespoints
set xlabel 'Time (ns)'
set ylabel 'Disk Block Address'
set key center top outside horizontal
plot data every 200:1 using 1:2 t label lw 1 lc rgbcolor "#ffc000"