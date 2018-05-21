#!/bin/bash
#
# Script to scan sysfs params for installed UTA boards and, if .bin file is newer than 
#  the image currently installed on the board, flash the new .bin file
#
# Usage:
#   fpga_flash [disable_flash] [force_flash]
#
# Options:
#    -disable_flash = Do *NOT* flash, or even prompt user to flash. 
#                        -Instead, exit with nonzero val if flash is required 
#                        -Note that this overrides all subsequent options
#
#    -force_flash = Do not check src/installed revIDs
#                        -Instead, proceed with flash process using latest .bin in src tree
#

#---------------------------------------------------------------------------
# Canonicalize Rev ID - return unique value for given Rev ID which can be 
#    passed to a logical '<' operation
# canonicalize_rev_id() "$rev_id" 
#---------------------------------------------------------------------------
function canonicalize_rev_id()
{
    local retval
    local revID="$1"

    #Expected format: mmddyy.II                                                                                     
    # where II is a 2-digit code indicating the version num within the day                                          
    # It hasn't really been standardized whether II is hex or decimal, so we're going                               
    #  to throw an error here if the value is greater than 9.                                                       
    if ! [[ "$revID" =~ ^[0-1][0-9][0-3][0-9][0-9][0-9]\.[0-9,a-f,A-F][0-9,a-f,A-F]$ ]] ; then #invalid format                          
        echo 0
        return
    fi

    local month=${revID:0:2}
    local day=${revID:2:2}
    local year=${revID:4:2}
    local idx=${revID:7:2}
    
    #bash interprets leading zeros to mean 'base=octal', remove them
    month=${month#0}
    day=${day#0}
    year=${year#0}
    idx=${idx#0}

    retval=$(((($year*365+($month-1)*31+$day)*256)+$idx))
    echo $retval
}

######## MAIN #########

DISABLE_FLASH="$2"
FORCE_FLASH="$3"
ERR=0
NUM_TO_FLASH=0
NUM_SKIPPED=0
NUM_FAILED=0

### Globals/Constants ###
DRIVER_PATH=/sys/$1
DRIVER_BOARD_PATH=$DRIVER_PATH/board
DRIVER_FPGA_PATH=$DRIVER_PATH/fpga

FLASH_WORKING_IMG=137    #Working Image Code
FLASH_GOLDEN_IMG=731    #Working Image Code

echo -e "\nChecking for hardware updates..."

## Get number of boards
BOARD_COUNT_FILE=$DRIVER_BOARD_PATH/board_count
BOARD_COUNT=`cat $BOARD_COUNT_FILE`

## Assert numboards == num fpgas
FPGA_COUNT_FILE=$DRIVER_FPGA_PATH/fpga_count
if [[ $BOARD_COUNT -ne `cat $FPGA_COUNT_FILE` ]] ; then
    echo 1>&2 "ERROR: Board Count != FPGA Count"
    exit 1
fi

declare -a BOARD_IDS
declare -a SRC_FILES
declare -a DO_FLASH

## foreach board
for BOARD in `seq 0 $(($BOARD_COUNT-1))`;
do

#### get board name
    BOARD_NAME=`cat $DRIVER_BOARD_PATH/$BOARD/board_name`

#### get installed revid
    CURR_REV_ID=`cat $DRIVER_FPGA_PATH/$BOARD/version`
    CURR_REV_IDX=$(canonicalize_rev_id $CURR_REV_ID)
    if [[ $CURR_REV_IDX -eq 0 && -z "$FORCE_FLASH" ]] ; then 
	echo 1>&2 "ERROR: Invalid REV ID read from '$DRIVER_FPGA_PATH/$BOARD/version'"
	ERR=1
	continue;
    fi

#### get source img revid

#
#  Parse the src/hw directory for the latest .bin for the proper board and get the REV ID
#
#        javelin_release_MMDDYYII.bin
#        0123456789012345678901234567890123456789
#        nemo_release_MMDDYYII.bin
#

    REV_ID_LEN=8
    if [[ $BOARD_NAME == "javelin" ]] ; then
	SUFFIX_IDX=24
	REV_ID_IDX=16
    elif [[ $BOARD_NAME == "nemo" ]] ; then
	SUFFIX_IDX=21
	REV_ID_IDX=13
    elif [[ $BOARD_NAME == "fin" ]] ; then
	SUFFIX_IDX=20
	REV_ID_IDX=12
    else
	echo 1>&2 "ERROR: Unsupported board [index=$BOARD, name=$BOARD_NAME] discovered"
	ERR=1
	continue
    fi

    SRC_REV_IDX=0
    for f in $OPENCL_TOP/src/hw/${BOARD_NAME}_release_*.bin
    do
	CURR_SRC_BIN_FILE=$f
	CURR_SRC_BIN_FILE_BASENAME=`basename $CURR_SRC_BIN_FILE`
	if [[ ${CURR_SRC_BIN_FILE_BASENAME:$SUFFIX_IDX} != ".bin" ]] ; then #bail out, something's not right
	    echo 1>&2 "ERROR: Invalid .bin filename [$CURR_SRC_BIN_FILE_BASENAME]"
	    ERR=1
	    continue
	fi
	CURR_SRC_REV_ID=${CURR_SRC_BIN_FILE_BASENAME:$REV_ID_IDX:$REV_ID_LEN}
	CURR_SRC_REV_ID="${CURR_SRC_REV_ID:0:6}.${CURR_SRC_REV_ID:6}" #add '.' between mmddyy & idx; canonicalize_rev_id() expects this 
	CURR_SRC_REV_IDX=$(canonicalize_rev_id $CURR_SRC_REV_ID)
	if [[ $CURR_SRC_REV_IDX -eq 0 ]] ; then 
	    echo 1>&2 "ERROR: Invalid REV ID read from '$SRC_BIN_FILE_BASENAME'"
	    ERR=1
	    continue;
	fi

	if [[ $CURR_SRC_REV_IDX -gt $SRC_REV_IDX ]] ; then
	    SRC_REV_IDX=$CURR_SRC_REV_IDX
	    SRC_BIN_FILE=$CURR_SRC_BIN_FILE
	    SRC_BIN_FILE_BASENAME=$CURR_SRC_BIN_FILE_BASENAME
	fi
    done

    if [[ $SRC_REV_IDX -eq 0 ]] ; then 
	echo 1>&2 "ERROR: Invalid REV ID"
	ERR=1
	continue;
    fi

    BOARD_ID_STR="UTA Board #$BOARD [Type=$BOARD_NAME,Version=$CURR_REV_ID]";

#### Check if flash required
####
####  If the hardware's out of date and the user wants to flash now,
####   we first prompt about each card and merely save the answer.
####   Then we'll loop back through and issue all the flash commands
####   in parallel.
####

    if [[ $SRC_REV_IDX -gt $CURR_REV_IDX || -n "$FORCE_FLASH" ]] ; 
    then ## Flash Required
	if [[ -z "$DISABLE_FLASH" ]] ; then
	    echo -e "\n\t$BOARD_ID_STR needs to be flashed."
	    echo -e "\tThis will take some time and a power cycle is required upon completion."
	    echo -e "\tFlash $BOARD_ID_STR now?"
	    select yn in "Yes" "No"; do
		case $yn in
		    Yes )  
			NUM_TO_FLASH=$((NUM_TO_FLASH+1))
			DO_FLASH[$BOARD]=1
			BOARD_IDS[$BOARD]=$BOARD_ID_STR
			SRC_FILES[$BOARD]=$SRC_BIN_FILE
			echo -e "\t$BOARD_ID_STR will be flashed."
			break;;
		    No ) 
			NUM_SKIPPED=$((NUM_SKIPPED+1))
			DO_FLASH[$BOARD]=0
			BOARD_IDS[$BOARD]=$BOARD_ID_STR
			SRC_FILES[$BOARD]=""
			echo -e "\t$BOARD_ID_STR will **NOT** be flashed."
			break;;
		esac
	    done
	else
	    echo 1>&2 "ERROR: $BOARD_ID_STR needs to be flashed [with $SRC_BIN_FILE_BASENAME], but the operation is disabled"
	    ERR=1 
	fi
    elif [[ $SRC_REV_IDX -lt $CURR_REV_IDX ]] ; 
    then ## Someone flashed an image that hasn't been checked in to src/hw/; warn.
	echo -e 1>&2 "\tWARNING: $BOARD_ID_STR has newer HW image than $SRC_BIN_FILE_BASENAME"
    else
	: ## Source & Installed revs match, do nothing.
    fi
done 

### 
### Issue the flash command(s)
###

for BOARD in `seq 0 $(($BOARD_COUNT-1))`;
do
    if [[ ${DO_FLASH[$BOARD]} -eq 1 ]] ; then
	echo -e "\nFlashing ${BOARD_IDS[$BOARD]}... "
	echo -e "\tFile: ${SRC_FILES[$BOARD]}"
	echo -e "\tNote: this will take a while. Use 'tail -f /var/log/kern.log' to monitor progress."
	DRIVER_FLASH_PATH=$DRIVER_FPGA_PATH/$BOARD/flash
	DRIVER_SIZE_FILE=$DRIVER_FLASH_PATH/prog_size
	DRIVER_IMAGE_FILE=$DRIVER_FLASH_PATH/image_type
	DRIVER_BITFILE_FILE=$DRIVER_FLASH_PATH/bitfile
    DRIVER_INIT_FILE=$DRIVER_FLASH_PATH/init_flash

	wc -c < ${SRC_FILES[$BOARD]} > $DRIVER_SIZE_FILE ## Set Flash Size
	echo $FLASH_WORKING_IMG > $DRIVER_IMAGE_FILE ## Set Flash Mode
	cat $SRC_BIN_FILE > $DRIVER_BITFILE_FILE & ## Flash (in background)
    echo 1 > $DRIVER_INIT_FILE
    fi
done

wait

### 
### Check the results
###

echo ""

for BOARD in `seq 0 $(($BOARD_COUNT-1))`;
do
    if [[ ${DO_FLASH[$BOARD]} -eq 1 ]] ; then
	DRIVER_FLASH_PATH=$DRIVER_FPGA_PATH/$BOARD/flash
	DRIVER_STATUS_FILE=$DRIVER_FLASH_PATH/status

	if [[ "`cat $DRIVER_STATUS_FILE`" == "SUCCEEDED" ]] ; then 
	    echo "${BOARD_IDS[$BOARD]} flashed successfully."
	else 
	    echo 1>&2 "${BOARD_IDS[$BOARD]} flash FAILED."
	    NUM_FAILED=$((NUM_FAILED+1))
	    ERR=1
	fi	

    fi
done

echo ""

if [[ $NUM_FAILED -ne 0 ]] ; then
    echo 1>&2 "ERROR: $NUM_FAILED flashes failed."
    exit $ERR
fi

if [[ $ERR -eq 0 && $NUM_TO_FLASH -gt 0 ]] ; then
    echo "All flashes ($NUM_TO_FLASH) complete."
    echo "Changes will take effect after the next power cycle."
    echo ""
fi

if [[ $NUM_SKIPPED -ne 0 ]]; then
    echo 1>&2 "$NUM_SKIPPED flashes skipped. Proceeding with out-of-date hardware."
else
    if [[ $ERR -eq 0 && $NUM_TO_FLASH -eq 0 ]]; then
	echo "All boards ($BOARD_COUNT) are up-to-date."
    fi
fi

exit $ERR
