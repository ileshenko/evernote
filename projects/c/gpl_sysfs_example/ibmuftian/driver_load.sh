#!/usr/bin/env bash
declare IBMUFTIAN_DBG=0
declare BOARD_DBG=1
declare ENGN_DBG=2
declare FLASH_DBG=3
declare FPGA_DBG=4
declare HMC_DBG=5
declare I2C_DBG=6
declare INTR_DBG=7
declare IOCTL_DBG=8
declare PCI_DBG=9
declare BAR_DBG=10
declare SYSFS_DBG=11
declare UTILS_DBG=12
declare BD_DBG=13
declare MMAP_DBG=14
declare LOAD_DBG=15
declare FLASHING_DBG=16
declare DATA_DBG=17
declare INT_DBG=18
declare FULL_DBG=19
declare ERR_DBG=20
declare QUIT=21
declare LEN=21
declare -a default_debug=($LOAD_DBG $ERR_DBG)
declare -a val_arr=( $(for i in `eval echo {1..$((LEN))}`};do echo 0;done))
input=0

function join_by { local IFS="$1"; shift; echo "$*"; }

module=${1##*/} #extract basename of first arg
module=${module%.ko} #trim .ko extension from first arg
devices="$module"
mode="664"

# Group: since distributions do it differently, look for wheel or use staff
if grep '^staff:' /etc/group > /dev/null; then
    group="staff"
else
    group="wheel"
fi

mod=`lsmod | grep $module | awk {'print $1'}`

if [ "$mod" = "$module" ]; then
    rmmod $module 
    if [[ $? -ne 0 ]]; then
	echo "Failed to unload $module" >&2
	exit 1
    fi
fi

if [ -z $2 ];then
    for ((i=0;i<${#default_debug[@]};i++))
    do
        val_arr[${default_debug[$i]}]=1
    done
else
    options=("IBMUFTIAN_DBG" "BOARD_DBG" "ENGN_DBG" "FLASH_DBG" "FPGA_DBG"
             "HMC_DBG" "I2C_DBG" "INTR_DBG" "IOCTL_DBG" "PCI_DBG" 
             "BAR_DBG" "SYSFS_DBG" "UTILS_DBG" "BD_DBG" "MMAP_DBG" "LOAD_DBG"
             "FLASHING_DBG" "DATA_DBG" "INT_DBG" "FULL_DBG" "ERR_DBG"
             "Quit")
    
    for ((i=1;i<=$LEN+1;i++))
    do
        echo $i":"${options[$i-1]}
    done | column
    
    while [ $input != $((QUIT+1)) ]
    do
        if [ $input != 0 ] && [ $input -le ${QUIT} ];then
            val_arr[$input-1]=1
        else
            if [ $input != 0 ];then
                echo "invalid input"
            fi
        fi
        read input
    done 
fi

debug_areas=$(join_by , "${val_arr[@]}")
echo "sudo insmod $1 debug_areas=$debug_areas"

sudo insmod $1 debug_areas=$debug_areas

for device in $devices;
do
    major=`cat /proc/devices | grep $device -m 1 | awk '{print $1}'`
    [[ "$major" ]] || {
    echo "module $module did not load" >&2
    exit 1
    }
    fpga_cnt=`cat /sys/$module/fpga/fpga_count`
    for ((i=0; i<fpga_cnt; i++)) ; 
    do

        # remove stale nodes
        if [ -e /dev/${device}$i ]; then
            echo "rm /dev/${device}$i..."
            rm -f /dev/${device}$i
        fi 

        # make new nodes
        echo "mknod /dev/${device}$i c $major $i"
        mknod /dev/${device}$i c $major $i
        if [[ $? -ne 0 ]]; then
            echo "Failed to create device file for $device $i" >&2
            exit 1
        fi

        # give appropriate group/permissions
        chgrp $group /dev/${device}$i
        chmod $mode  /dev/${device}$i
    done
done
