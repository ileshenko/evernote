#!/bin/sh

if [ $# -ne 1 ]; then
	echo give cpu number
	exit 1
fi

printf "\033c"
grep --color \#$1 apps/pni/pni_target/work/simulator/logs/minicom_be.log

