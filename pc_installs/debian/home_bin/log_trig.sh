#!/bin/sh


OLD_TRIG=`cat ~/tmp/log_trigger`
sleep 9999 &
echo $! > ~/tmp/log_trigger
kill $OLD_TRIG

