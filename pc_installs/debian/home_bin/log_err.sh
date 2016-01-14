#!/bin/sh

if [[ -n $1 ]] ; then
	CPU=$1
elif [[ -a ~/tmp/next_cpu ]] ; then
	CPU=`cat ~/tmp/next_cpu`
else
	echo no cpu number
	exit 1
fi

if (( CPU < 256 )) ; then
LOG=apps/pni/pni_target/work/simulator/logs/minicom_be.log
else
LOG=apps/pni/pni_target/work/simulator/logs/minicom_dp.log
fi

while true ; do
printf "\033c"
TRIGGER=`cat ~/tmp/log_trigger`
echo errors from $LOG, trigger $TRIGGER
echo -en "\033[36m"
tail --pid=$TRIGGER -F $LOG | grep -v -e "PNIL#" -e "DP#"
sleep 1
done
