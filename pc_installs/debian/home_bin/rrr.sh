#!/bin/bash

lpt_pwr reset
sleep 3
echo q > /dev/ttyS0
sleep 1
#echo "nand read 0x42000000 0x1440000 0x500000 ; bootm start 0x42000000 ; bootm loados" > /dev/ttyS0
#sleep 1
#echo "nand read 0x42000000 0x1940000 0x500000 ; bootm start 0x42000000 ; bootm loados" > /dev/ttyS0
#sleep 1
echo "tftpboot 41000000 openrg.img ; bootm 41000000"  > /dev/ttyS0

