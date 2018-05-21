/*******************************************************************************
 * ibmuftian_ioctls.h -- Netezza, an IBM Company, Universal Throughput Accelerator
 *
 * Ioctl Interface
 * Author: Tyler Kenney <tjkenney@us.ibm.com>
 *
 *******************************************************************************
 * Copyright (c) 2011, Netezza, an IBM Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Netezza Corporation, an IBM company nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NETEZZA CORPORATION, AN IBM COMPANY, ''AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NETEZZA CORPORATION, AN IBM COMPANY, BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * You may contact Netezza, an IBM company, at 26 Forest Street
 * Marlborough, MA, 01752, USA.
 *
 *****************************************************************************/
#ifndef _IBMUFTIAN_IOCTLS_H_
#define _IBMUFTIAN_IOCTLS_H_

#ifdef __KERNEL__
# include <asm/ioctl.h>
#else
# include <sys/ioctl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
# define _COMPAT_IO(type,nr) _IOC(_IOC_NONE,(type),(nr),0)
# define _COMPAT_IOR(type,nr,size) _IOC(_IOC_READ,(type),(nr),sizeof(size))
# define _COMPAT_IOW(type,nr,size) _IOC(_IOC_WRITE,(type),(nr),sizeof(size))
#else

# define _COMPAT_IO  _IO
# define _COMPAT_IOR _IOR
# define _COMPAT_IOW _IOW
#endif // __cplusplus

#define UTA_AVAILABLE_FILENAME		"uta_available"
#define UTA_MEM_SIZE_FILENAME		"mem_size"
#define UTA_ENG_COUNT_FILENAME		"engn_count"
#define UTA_BOARD_COUNT_FILENAME	"fpga_count"

#define IBMUFTIAN_DEV_FILE_FMT_STR		"/dev/ibmuftian%u"
#define IBMUFTIAN_DEV_FILE_STR_LEN		sizeof(IBMUFTIAN_DEV_FILE_FMT_STR)

#define IBMUFTIAN_AVAILABLE_FMT_STR		"/sys/ibmuftian/fpga/%u/" UTA_AVAILABLE_FILENAME
#define IBMUFTIAN_AVAILABLE_STR_LEN		sizeof(IBMUFTIAN_AVAILABLE_FMT_STR)

#define IBMUFTIAN_ENG_COUNT_FMT_STR		"/sys/ibmuftian/fpga/%u/" UTA_ENG_COUNT_FILENAME
#define IBMUFTIAN_ENG_COUNT_STR_LEN		sizeof(IBMUFTIAN_ENG_COUNT_FMT_STR)

#define IBMUFTIAN_BOARD_COUNT_FMT_STR      "/sys/ibmuftian/fpga/" UTA_BOARD_COUNT_FILENAME
#define IBMUFTIAN_BOARD_COUNT_STR_LEN      sizeof(IBMUFTIAN_BOARD_COUNT_FMT_STR)

#define IBMUFTIAN_MEM_SIZE_FMT_STR		"/sys/ibmuftian/fpga/%u/bar/2/" UTA_MEM_SIZE_FILENAME
#define IBMUFTIAN_MEM_SIZE_STR_LEN		sizeof(IBMUFTIAN_MEM_SIZE_FMT_STR)

/**
 * Fundamental Preconditions:
 *
 * 1.) It is assumed that the initialization routine for the ibmuftian module will
 * 		return success if and only if:
 * 			a.) a compatible PCIe card is found in the system (either Nemo or Nallatech 385-n Card)
 * 			b.) The FPGA is configured with a valid UTA image
 *
 * 2.) The module will only be loaded with a script (driver_load) which creates the character-device file:
 * 			<IBMUFTIAN_DEV_FILE>
 *
 * 		This script will be very similar to nzds_load, and function by parsing /proc/devices
 * 		for ibmuftian's dynamically allocated major number and then using this to create the
 * 		dev file
 *
 *
 * 	Therefore, the presence of:
 * 		-the ibmuftian module in the kernel (as read by lsmod) OR
 * 		-the dev file <IBMUFTIAN_DEV_FILE>
 *
 * 		implies that a compatible board with a valid UTA image is installed in the system.
 *
 * 3.) The open() method defined for the device file <IBMUFTIAN_DEV_FILE> should permit only 1 open
 *            file descriptor at a time (at least for now, we're just going to have 1 process using 
 *            the entire UTA at a time, so let's lock everybody else out to be safe). 
 *
 *            NOTE: This can be implemented with the atomic_t data type
 *
 *            NOTE: If one open() call has already succeeded [without a corresponding close()],
 *               the open() implementation should return -EBUSY
 *
 * 4.) The module will export read-only parameters to user space via the sysfs interface
 * 		using the module_param() macro. Currently, the exported parameters are:
 *
 * 			<IBMUFTIAN_ENG_COUNT_FILE> = The number of UTA engines in the device
 *          <IBMUFTIAN_AVAILABLE_FILE> = Zero when unavailable (as defined above in req #3),
 *                                          non-zero when available
 *          <IBMUFTIAN_MEM_SIZE_FILE>  = The size, in bytes, of memory available on the UTA card
 *
 * 5.) The module will export other read-only parameters to user space via the sysfs interface                                
 *          <IBMUFTIAN_BOARD_NAME>     = board name
 *
 *          <IBMUFTIAN_FPGA_VERSION>  = Fpga version, format "MMDDYY.REV" like "092315.02"
 *
 *
 *
 *      The IOCTL commands defined in this file will be invoked using the ioctl() system call
 *      with the file descriptor returned by an open() call on <IBMUFTIAN_DEV_FILE>
 *
 **/


/**
 * The magic number used for kernel ioctl's should be an unregistered value
 * so that the resulting IOCTL command number will be unique. We'll keep with
 * the convention of nzds and use ascii 'Z' - but we won't duplicate sequence
 * numbers to avoid collisions in the rare case that nzds and ibmuftian get loaded
 * into the kernel concurrently.
 *
 * nzds uses sequence numbers 1-8 & 33-37
 * Let's use 60-79 for ibmuftian
*/
#define IBMUFTIAN_MAGIC 'I'

/**
 * IOCTL Command: UTA_WR_MEM_EXEC_ENG [UTA Write Memory & Execute Engine]
 *
 * This command's parameter is a user-space pointer to a uta_wr_mem_exec_eng_t struct.
 *
 * This command performs 3 high-level steps:
 * 	  1.) Populate an IBD with the given data [as outlined below]
 *    2.) Ring the doorbell register of engine #<eng_num> with the address of the populated IBD
 *    3.) Block the calling thread until the next execution-completion interrupt is received from engine #<eng_num>
 *
 *    IBD Fields:
 *    	1. Global Memory [Either DDR or HMC]
 *    			-All 4KB physical pages containing data from the buffer pointed to by the virtual address <data.host_ptr>
 *    			  of length <data.buff_size> should be pinned & copied to contiguous UTA global memory beginning at
 *    			  address <data.uta_ptr>
 *      2. Instruction Queue
 *      		-All 4KB physical pages containing data from the buffer pointed to by the virtual address <text.host_ptr>
 *    			  of length <text.buff_size> should be pinned & copied to the beginning of the instruction queue of
 *    			  engine #<eng_num>
 *      3. GPRs
 *      		-<reg_init.enable_mask> should be copied into the 2nd control word of the IBD GPR section
 *      		-the 256 bytes in <reg_init.reg_val> should be copied into the 256 byte payload of the IBD GPR section
 *      4. Completion interrupt enabled
 *
 *
 * NOTE: This command should block until the engine execution has completed. This can be implemented
 *  by sleeping on a wait queue with a flag that is set in the interrupt handler for the engine
 *  completion interrupt
 *
 * NOTE: The exact specification for the GPR field of the IBD is not in the NEMO spec as of this writing. Mike Healy
 *    will update the spec as soon as possible.
 *
 */
#define UTA_WR_MEM_EXEC_ENG		_COMPAT_IOW(IBMUFTIAN_MAGIC,71,uta_wr_mem_exec_eng_t)

/**
 * IOCTL Command: UTA_WR_MEM [UTA Write Memory]
 *
 * This command's parameter is a user-space pointer to a uta_mem_transfer_t struct.
 *
  * This command performs 6 high-level steps:
 * 	  1.) let NUM_IBDS = <buff_size> / (MAX_GLOBAL_MEM_DATA_PER_IBD)
 * 	  2.) Populate <NUM_IBDS> IBDs with the given data [as outlined below]
 * 	  3.) let i = 0
 *    4.) Ring the doorbell register of engine #0 with the address of the ith populated IBD
 *    5.) Block the calling thread until the next global-mem-write-completion interrupt is received from engine #0
 *    6.) if i < NUM_IBDs, go to step #4
 *        else, return success
 *
 *    IBD Fields:
 *    	1. Global Memory [Either DDR or HMC]
 *    			-All 4KB physical pages containing data from the buffer pointed to by the virtual address <host_ptr>
 *    			  of length <buff_size> should be pinned & copied to contiguous UTA global memory beginning at
 *    			  address <uta_ptr>
 *      2. Completion interrupt enabled
 *
 *
 * NOTE: MAX_GLOBAL_MEM_DATA_PER_IBD is ~1MB, but this is subject to change. See latest revision of the Nemo HW spec
 *
 * NOTE: We are potentially going to add an out-of-band (i.e., no associated UTA engine) DMA to handle IBDs in which
 *   global memory is the only valid field. Until then, however, we are going to send all global mem read/write IBDs
 *   to engine 0.
 *
 * NOTE: It is imperative that a separate thread is able to write an engine-execution IBD to engine #0 and have it begin
 *   with minimal latency. This is why the above steps require only writing 1 IBD to the engine's input queue at a time,
 *   this will always leave room for an execution IBD and there should be no more than 1 IBD ahead of it in engine 0's queue.
 *
 */
#define UTA_WR_MEM				_COMPAT_IOW(IBMUFTIAN_MAGIC,72,uta_mem_transfer_t)

/**
 * IOCTL Command: UTA_RD_MEM [UTA Read Memory]
 *
 * TThis command's parameter is a user-space pointer to a uta_mem_transfer_t struct.
 *
  * This command performs 6 high-level steps:
 * 	  1.) let NUM_OBDS = <buff_size> / (MAX_GLOBAL_MEM_DATA_PER_OBD)
 * 	  2.) Populate <NUM_OBDS> OBDs with the given data [as outlined below]
 * 	  3.) let i = 0
 *    4.) Ring the doorbell register of engine #0 with the address of the ith populated OBD
 *    5.) Block the calling thread until the next global-mem-read-completion interrupt is received from engine #0
 *    6.) if i < NUM_OBDs, go to step #4
 *        else, return success
 *
 *    OBD Fields:
 *    	1. Global Memory [Either DDR or HMC]
 *    			-All 4KB physical pages containing data from the buffer pointed to by the virtual address <host_ptr>
 *    			  of length <buff_size> should be pinned & filled with data read from contiguous UTA global memory
 *    			  beginning at address <uta_ptr>
 *      2. Completion interrupt enabled
 *
 *
 * NOTE: MAX_GLOBAL_MEM_DATA_PER_OBD is ~1MB, but this is subject to change. See latest revision of the Nemo HW spec
 *
 * NOTE: We are potentially going to add an out-of-band (i.e., no associated UTA engine) DMA to handle OBDs in which
 *   global memory is the only valid field. Until then, however, we are going to send all global mem read/write OBDs
 *   to engine 0.
 *
 * NOTE: It is imperative that a separate thread is able to write an engine-execution IBD to engine #0 and have it begin
 *   with minimal latency.
 *
 */
#define UTA_RD_MEM				_COMPAT_IOW(IBMUFTIAN_MAGIC,73,uta_mem_transfer_t)

#define FPC_CONFIG				_COMPAT_IOW(IBMUFTIAN_MAGIC,64,fpc_block_t)
/**
 * IOCTL Command arguments
 */
typedef struct uta_wr_gpr
{
	unsigned int enable_mask;
	unsigned long reg_val[32];
} uta_wr_gpr_t;

typedef struct uta_mem_transfer
{
	unsigned long long buff_size;
	void* host_ptr;
	unsigned long uta_ptr;

	/* Debug Info */
	unsigned long uta_buff_origin; //differs from uta_ptr when we're writing to an offset within a cl_mem
	unsigned long uta_buff_total_len; //differs from 'buff_size' when given command doesn't write to every byte that was allocated for a cl_mem

} uta_mem_transfer_t;

typedef struct uta_wr_iq
{
	unsigned int buff_size;
	void* host_ptr;
} uta_wr_iq_t;

typedef struct uta_wr_mem_exec_eng
{
	unsigned char eng_num;
	uta_wr_iq_t text;
	uta_wr_gpr_t reg_init;
	uta_mem_transfer_t data;
} uta_wr_mem_exec_eng_t;

typedef struct fpc_block
{
	unsigned int buff_size;
	void* host_ptr;
} fpc_block_t;

#ifdef __cplusplus
};
#endif


#endif //_IBMUFTIAN_IOCTLS_H_
