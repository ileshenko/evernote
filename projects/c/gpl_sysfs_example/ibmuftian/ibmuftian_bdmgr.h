#ifndef _IBMUFTIAN_BDMGR_H_
#define _IBMUFTIAN_BDMGR_H_

#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/kfifo.h>
#include <linux/semaphore.h>
#include <linux/timer.h>

#include "ibmuftian_utils.h"

struct ibmuftian_fpga;
struct ibmuftian_engn;

typedef struct bd_length
{
    union
    {
        u32 all;
        struct
        {
            u32 length : LEN_BITS;   // [9:0]   
            u32 reserved : 22;       // [31:10]  
        } bit;
    };

} __attribute__ ((packed)) bd_length;

typedef struct status_buffer
{
    struct page *status_buff;
    dma_addr_t  status_buff_addr;
} status_buffer;

typedef struct bd_addr
{
    void * vir_add;
    dma_addr_t dma_add;
} bd_addr;

typedef struct bd_book_keeping
{
    status_buffer status_buffs[BD_TYPES];
    struct kfifo bd_fifo[BD_TYPES];
    struct semaphore bd_fifo_sema[BD_TYPES];
    bd_addr bd_add[BD_TYPES];
    spinlock_t fifo_lock[BD_TYPES];    
    int bd_in_queue[BD_TYPES];
    int inst_job;
} bd_book_keeping;

typedef struct mem_move{
	void *src;
	void *dst;
	unsigned int offset;
	unsigned long len;
	unsigned int count;
} mem_move;

typedef struct regs_write{
	unsigned int enable_mask;
	unsigned long *reg_val;
} regs_write;

typedef enum
{
    INIT,
    IN_PROG,
    BD_DONE,
    BD_FAILED,
    TIME_OUT
} buff_des_state;

typedef struct bd_bookeeping
{
    unsigned short bd_num;
    buff_des_state bd_state;
    struct inst_bookeeping
    {
        unsigned complete;
        struct completion ibd_completion;
    } inst_bookeeping;
    int set_interrupt;
    int error_status;
    void *parent_job;
    int bd_type;
    int inst_mem;
    unsigned int done;
    regs_write gprs;
    union
    {
        mem_move inst;
	    mem_move data;
    } mem_addr;
    struct sg_table *sg_data;
    bd_addr bd_mem_ptr;
    bd_length bd_len;
    dma_addr_t inst_dma_addr;
    u32 size;
    struct timer_list bd_timer;
} bd_bookeeping;

typedef struct ibmuftian_job
{
    struct pci_dev *pdev;
    int engn_no;
    unsigned int total_bd;
    int job_type;
    struct completion job_completion;
    int error_stat;
    int job_result;
    atomic_t bd_count;
    bd_bookeeping *bd;
    unsigned short bd_tag_start_id;
    int redo;
    struct list_head list;
} ibmuftian_job;

typedef struct inst_bd
{
    u32 src_host_addr_hi;
    u32 src_host_addr_lo;
	u32 gpr_mask;
	struct gpr_reg{
		u32 gpr_val_high;
		u32 gpr_val_low;
	} __attribute__ ((packed)) gpr_reg[BD_MAX_GPR_REGS];
} __attribute__ ((packed)) inst_bd;

typedef struct mem_ibd_bd
{
    u32 dst_hmc_addr_hi;
    u32 dst_hmc_addr_lo;
    struct src_addr
    {
        u32 src_host_addr_hi;
        u32 src_host_addr_lo;
    } __attribute__ ((packed)) src_addr[BD_MAX_HOST_PTR_REGS];
} __attribute__ ((packed)) mem_ibd_bd;

typedef struct mem_obd_bd
{
    u32 src_hmc_addr_hi;
    u32 src_hmc_addr_lo;
    struct dest_addr
    {
        u32 dst_host_addr_hi;
        u32 dst_host_addr_lo;
    } __attribute__ ((packed)) dest_addr[BD_MAX_HOST_PTR_REGS];
}__attribute__ ((packed)) mem_obd_bd;

typedef struct buffer_desc
{
    union
    {
        u32 all;
        struct
        {
            u32 desc_id : DESC_ID_LEN;          // [15:0]   DESCRIPTOR_ID
            u32 length : LEN_BITS;              // [25:16]  IBD_DESCRIPTOR_LENGTH_DW
            u32 reserved : 4;                   // [29 26]
            u32 desc_int_enable : INTR_BIT_LEN; // [30] 0=Do Not Set Interrupt when Descriptor Completes 1=Set Interrupt when Descriptor Completes
            u32 hmc_instq : TYPE_BIT_LEN;       // [31] 0=hmc 1=instq/gpr
        } bit;
    } __attribute__ ((packed)) control_bits;
    u32 total_len;
	union
	{
       inst_bd inst;
       mem_ibd_bd mem_ibd;
       mem_obd_bd mem_obd;
	} __attribute__ ((packed)) bd_mem_element;
}__attribute__ ((packed)) buffer_desc;

typedef struct bd_result
{
    union
    {
        u32 dword[STATUS_BUFF_LEN_DWORD];
        struct
        {  
            u32 ibd_id : DESC_ID_LEN;         // Words (Bytes) moved for this job
            u32	reserved : 16;
            u32 dma_len;
            u32 job_dur_upper;
            u32 job_dur_lower;
            u32 timer_up;
            u32 timer_low;
            u32 dword_reserved[RESERVED_DWORDS_IN_STATUS];
        } dword_distribution;
    } buff_desc_result;

}__attribute__ ((packed)) bd_result;

void delete_status_buffer(struct ibmuftian_engn *, int);
int init_status_buffer(struct ibmuftian_engn *, int);
int init_bd_mem(struct ibmuftian_engn *, int);
void free_bd_mem(struct ibmuftian_engn *, int);
void delete_bd_fifo(struct ibmuftian_engn *, int);
int init_bd_fifo(struct ibmuftian_engn *, int);
void delete_bd_book_keeping(struct ibmuftian_engn *, int);
int init_bd_book_keeping(struct ibmuftian_engn *, int);
void get_bd(struct ibmuftian_engn *, int );
void return_bd(struct ibmuftian_engn *, int );
int build_send_bd(struct ibmuftian_engn *, ibmuftian_job *);
void reset_and_return_bd(struct ibmuftian_engn *, bd_bookeeping *);
void bd_status_ckeck_cleanup(struct ibmuftian_engn *, bd_result *, int);
void bd_timed_out(unsigned long);
void reset_fifo_status_buffer(struct ibmuftian_engn *, int);

#endif

