#!/bin/sh
#
# Wrapper around make, to colorize it's output and pipe through less.
# Jumps to the first gcc error that occurs during the build process.
#

echo $1
if [ $# ==  1 ]; then
  a=$1
else
  a=0
fi

#if (($PIPESTATUS)); then

if [ $a !=  0 ]; then
  rt beep -f 1800 -n -f 1600 -n -f 1400 -n -f 1200  -n -f 1000 
#  aplay /usr/share/kde4/apps/ktouch/down.wav
  aplay /usr/share/kde4/apps/blinken/sounds/4.wav
  aplay /usr/share/kde4/apps/blinken/sounds/3.wav
  aplay /usr/share/kde4/apps/blinken/sounds/2.wav
  aplay /usr/share/kde4/apps/blinken/sounds/1.wav
else
  rt beep -f 1000 -n -f 1200 -n -f 1400 -n -f 1600  -n -f 1800 
#  aplay /usr/share/kde4/apps/ktouch/up.wav
  aplay /usr/share/kde4/apps/blinken/sounds/[1-4].wav
fi

date
exit $PIPESTATUS
