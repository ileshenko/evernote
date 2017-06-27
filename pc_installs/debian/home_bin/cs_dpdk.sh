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
#find "$PWD/dpe" "$PWD/samples/pci_stack/pci_stack_app/src" -name "*.[chxsS]" -print > cscope.files
#find "/home/npsdb/br/ldk/x86_64/linux" "$PWD/ldk/build/nps_transp" -name "*.[chxsS]" -print > cscope.files
#find "$PWD/dpe" "$PWD/apps/pci_tr" -name "*.[chxsS]" -print > cscope.files

find . "/usr/include" -path "*/build" -prune -o -name "*.[chxsS]" -print > cscope.files
echo "build/include/rte_config.h" >> cscope.files
cscope -b
ctags -R --exclude="build"
ctags -a "build/include/rte_config.h"
rm -f cscope.files

