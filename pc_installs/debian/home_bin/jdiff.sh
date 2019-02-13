#!/bin/bash

git diff --name-only $*^ $*
git difftool $*^ $*
#printf %x\\n $(($*))
#printf %x\\n $((0x$2 - $1))
