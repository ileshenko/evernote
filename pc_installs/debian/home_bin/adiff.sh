#!/bin/bash

#printf %x\\n $(($*))
printf %x\\n $((0x$2 - $1))
