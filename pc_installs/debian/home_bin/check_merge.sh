#!/bin/bash

LIST=/tmp/$1_m_list2.txt
LIST_SH=/tmp/$1_m_list2.sh
LIST_SH_NO_OK=/tmp/$1.no_ok.sh
LIST_DIFF_NO_OK=/tmp/$1.no_ok.txt
LIST_DIFF_OK=/tmp/$1.ok.txt
REVIEW_SH=$1.sh
REVIEW_LIST=$1.list
export REVIEW_LIST

rm $LIST $LIST_SH $LIST_DIFF_OK $LIST_DIFF_NO_OK $LIST_SH_NO_OK
cat $REVIEW_SH | grep "cvs -f up" | cut -f 6,8,9 -d ' ' > $LIST
sed -e 's/\(.*\) \(.*\) \(.*\)/rmerge1\.sh \3 \1 \2 /g' $LIST > $LIST_SH
chmod +x $LIST_SH
export LIST_DIFF_OK
export LIST_DIFF_NO_OK
echo "First Run started"
$LIST_SH
sed -e 's/\(.*\) \(.*\) \(.*\)/rmerge1\.sh \3 \1 \2 /g' $LIST_DIFF_NO_OK | uniq > $LIST_SH_NO_OK
chmod +x $LIST_SH_NO_OK
LIST_DIFF_OK=
LIST_DIFF_NO_OK=
export LIST_DIFF_OK
export LIST_DIFF_NO_OK
echo
echo "Second Run started"
$LIST_SH_NO_OK
