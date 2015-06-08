#!/bin/sh

input=$1

grep -w CPU#16 $input > ~/tmp/cpu_16.log
grep -wv CPU#16 $input > ~/tmp/cpu_r.log
mv ~/tmp/cpu_r.log ~/tmp/cpu_rest.log

grep -w CPU#17  ~/tmp/cpu_rest.log > ~/tmp/cpu_17.log
grep -wv CPU#17  ~/tmp/cpu_rest.log > ~/tmp/cpu_r.log
mv ~/tmp/cpu_r.log ~/tmp/cpu_rest.log

grep -w CPU#18 ~/tmp/cpu_rest.log > ~/tmp/cpu_18.log
grep -wv CPU#18 ~/tmp/cpu_rest.log > ~/tmp/cpu_r.log
mv ~/tmp/cpu_r.log ~/tmp/cpu_rest.log

grep -w CPU#0 ~/tmp/cpu_rest.log > ~/tmp/cpu_0.log
grep -wv CPU#0 ~/tmp/cpu_rest.log > ~/tmp/cpu_r.log
mv ~/tmp/cpu_r.log ~/tmp/cpu_rest.log

grep -w CPU#1 ~/tmp/cpu_rest.log > ~/tmp/cpu_1.log
grep -wv CPU#1 ~/tmp/cpu_rest.log > ~/tmp/cpu_r.log
mv ~/tmp/cpu_r.log ~/tmp/cpu_rest.log

gvim -p -c "set co=160" ~/tmp/cpu_*.log 
