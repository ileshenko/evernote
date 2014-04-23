#!/bin/bash

cd  build.JPKG_ARMV7/jpkg
../pkg/jpkg/jpkgcreate -p $1
\cp rg-$1_* ../../jpkgs_dir/ -v
cd ../..
jpkg-x  jpkgs_dir/rg-$1_*
#printf %x\\n $(($*))
#printf %x\\n $((0x$2 - $1))
