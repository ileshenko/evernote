#!/bin/bash

rm -rf release
mkdir release

for i in `find -name "*.[ch]"`; do cp $i release/$i ; done

G_DATA=`cat gnu_lic.txt`
B_DATA=`cat bsd_lic.txt`
D_DATA=`cat dual_lic.txt`

cd release

for i in `find -name "*.[ch]"`; do \
gawk -v gnu="$G_DATA" -v bsd="$B_DATA" -v dual="$D_DATA" \
'{sub(/LICENSE_CODE_MLNX_GPL/,gnu);
  sub(/LICENSE_CODE_MLNX_BSD/,bsd);
  sub(/LICENSE_CODE_MLNX_DUAL/,dual)}; { print }' $i > tmp.tmp ; mv -f tmp.tmp $i; done
