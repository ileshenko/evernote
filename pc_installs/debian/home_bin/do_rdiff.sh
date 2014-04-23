#!/bin/sh

if [ "-e" == "$2" ]; then
    minusb=$3
    branch=$4
    if [ -e "$1.sh" ]; then
	exist=y
    fi
else
    minusb=$2
    branch=$3
fi

if [ "$exist" != "y" ]; then
    review diff -j $minusb $branch $1 | grep -v "U " > $1.sh
    chmod +x $1.sh
    cat $1.sh | grep cvs | cut -f 9 -d ' ' > $1.list
fi
