#!/bin/sh
# http://cscope.sourceforge.net/large_projects.html

PWD=`pwd`

##	-path "$LNX/arch/*" ! -path "$LNX/arch/i386*" -prune -o               \
##	-path "$LNX/include/asm-*" ! -path "$LNX/include/asm-i386*" -prune -o \
##	-path "$LNX/tmp*" -prune -o                                           \
##	-path "$LNX/Documentation*" -prune -o                                 \
##	-path "$LNX/scripts*" -prune -o                                       \
##	-path "$LNX/drivers*" -prune -o                                       \
##      -name "*.[chxsS]" -print >/home/jru/cscope/cscope.files

#find "$PWD/apps/pcie" "$PWD/dpe" "$PWD/samples/pci_stack/pci_stack_app/src" -name "*.[chxsS]" -print > cscope.files
find "$PWD/dpe" "$PWD/samples/pci_stack/pci_stack_app/src" -name "*.[chxsS]" -print > cscope.files

