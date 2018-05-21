#ifndef _IBMUFTIAN_IOCTL_H_
#define _IBMUFTIAN_IOCTL_H_
#include <asm/uaccess.h>
#include <linux/ktime.h>
#include <linux/delay.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_bdmgr.h"

struct ibmuftian_fpga;

typedef struct job_book_keeping
{
    struct list_head job_queue;
    spinlock_t job_queue_lock;
} job_book_keeping;

typedef struct mem_buff
{
	unsigned long long buff_size;
	void* host_ptr;
	unsigned long uta_ptr;

	/* Debug Info */
	unsigned long uta_buff_origin; //differs from uta_ptr when we're writing to an offset within a cl_mem
	unsigned long uta_buff_total_len; //differs from 'buff_size' when given command doesn't write to every byte that was allocated for a cl_mem

} mem_buff;

typedef struct inst_buff
{
	unsigned int buff_size;
	void* host_ptr;
} inst_buff;
    
typedef struct gprs
{
	unsigned int enable_mask;
	unsigned long reg_val[32];
} gprs;

typedef struct sgt_container
{
	struct sg_table *sgt;
	unsigned long sgt_len;
	unsigned long sgt_offset;
} sgt_container;


typedef struct ioctl_inst
{
	unsigned char eng_num;
	inst_buff inst;
	gprs reg_init;
	mem_buff data;
} ioctl_inst;

int ioctl_call(struct ibmuftian_fpga *, unsigned int, unsigned long);
int wr_exec_req(struct ibmuftian_fpga *, unsigned long);
int wr_mem_req(struct ibmuftian_fpga *, unsigned long);
int rd_mem_req(struct ibmuftian_fpga *, unsigned long);

#endif
