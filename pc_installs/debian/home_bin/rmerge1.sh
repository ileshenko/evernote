#!/bin/bash

#REPLACE_FILES=`grep -F $1 $REVIEW_LIST | cut -f2- -d ' '`
REPLACE_FILES=$1

strip_diffs()
{
    sed -e 's/@@\(.*\)@@/@@ HUNK @@/g' $1 | grep -v "+++" | grep -v "\-\-\-" | grep -v "retrieving revision" | grep -v "diff \-u " | grep -v "diff \-N " | grep -v "RCS file:" | grep -v "Index: " | grep -v "======" | grep -v "\$Header.*\$" > $2
}

for REPLACE_FILE in $REPLACE_FILES
do
    echo $1 \<\-\> $REPLACE_FILE
    mkdir -p /tmp/rmerge/`dirname $1`
    mkdir -p /tmp/rmerge/`dirname $REPLACE_FILE`
    cvs rdiff -kk -u -r $2 -r $3 rg/$1 > /tmp/rmerge/$1.1_
    cvs diff -uN $REPLACE_FILE > /tmp/rmerge/$REPLACE_FILE.2_

    strip_diffs /tmp/rmerge/$1.1_ /tmp/rmerge/$1.1
    strip_diffs /tmp/rmerge/$REPLACE_FILE.2_ /tmp/rmerge/$REPLACE_FILE.2

    if [ -z "$LIST_DIFF_NO_OK" ] ; then
	gvim -df /tmp/rmerge/$1.1 /tmp/rmerge/$REPLACE_FILE.2
    else
	diff /tmp/rmerge/$1.1 /tmp/rmerge/$REPLACE_FILE.2 > /dev/null
	if [ 1 -eq $? ] ; then
	    echo $2 $3 $1 >> $LIST_DIFF_NO_OK
	else
	    echo $1 >> $LIST_DIFF_OK
	fi
    fi
done
