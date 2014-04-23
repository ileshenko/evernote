#!/bin/bash

q=`basename $1 .bmp`

echo $q

bmptoppm $1 > $q.ppm
pnmscale 2 $q.ppm >$q-2.ppm
ppmtopgm $q-2.ppm >$q-2.pgm
pgmtopbm $q-2.pgm >$q-2.pbm
pnmtojpeg --quality=10 $q-2.pbm >$q.jpg

