#!/bin/sh

if [[ -n "$1" ]]; then
	CPU=$1
elif [[ -a ~/tmp/next_cpu ]]; then
	CPU=`cat ~/tmp/next_cpu`
else
	echo no cpu number
	exit 1
fi

NEXT_CPU=`echo $(($CPU + 1))`
echo $NEXT_CPU > ~/tmp/next_cpu


if (( CPU < 256 )) ; then 
#LOG=apps/pni/pni_target/work/simulator/logs/minicom_be.log
LOG=/tmp/ezbox26_telnet
else
LOG=apps/pni/pni_target/work/simulator/logs/minicom_dp.log
fi

while true ; do
printf "\033c"
TRIGGER=`cat ~/tmp/log_trigger`
echo filtering CPU $CPU, trigger $TRIGGER
if (( CPU < 256 )) ; then 
tail --pid=$TRIGGER -F $LOG | sed -u 's/^.\{13\}//' | grep --color \#$CPU
else
tail --pid=$TRIGGER -F $LOG | grep --color \#$CPU
fi

# | grep --line-buffered --color \#$CPU
sleep 1
done
