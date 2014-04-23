#!/bin/bash

REVIEW=R$1
BRANCH=branch-5_4_8_1_62_1
BUG=B120092
FROM=5.4.8.1.62.1
TO=5.4.8.1.124.1

TITLE=`review diff $REVIEW | grep TITLE | sed 's/.*: *//'`
FILES=`review diff -b $BRANCH $REVIEW | grep "^cvs rdiff"| sed 's/.* rg\///'|sort|uniq`

review diff -b $BRANCH -j -x $REVIEW

echo ---------------
echo "$BUG: Merge $REVIEW ($TITLE) from $FROM to $TO"
echo review ci $REVIEW $FILES
