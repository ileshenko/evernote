#ifndef _IBMUFTIAN_H_
#define _IBMUFTIAN_H_

#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/moduleparam.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_sysfs.h"

extern int __read_mostly debug_areas[TOTAL_DBG];

#define IBMUFTIAN_MAGIC 'I'
#define DBG_PRAM(...) __VA_ARGS__
#define PRINT(dbg_prm, fmt, args...)\
        {\
            int i;\
            int prms[] = dbg_prm;\
            for(i = 0; i < sizeof(prms)/sizeof(int); i ++)\
            {\
                if (debug_areas[prms[i]])\
                {\
                    printk(KERN_INFO "IN file: %s func %s line.no: %d %*s" fmt "\n", __BASE_FILE__, __func__, __LINE__, SPACE, ##args);\
                    break;\
                }\
            }\
        }

typedef struct callback_funcs
{
    void(*engn_inuse)(int,int);
} callback_funcs;

typedef struct perf_attrs
{
    struct timeval total_time;
} perf_attrs;

typedef struct ibmuftian_driver
{
    struct kobject *kobj;
    ibmuftian_board_container *board_c;
    ibmuftian_fpga_container *fpga_c;
    ibmuftian_engn_container *engn_c;
    callback_funcs functions;
    perf_attrs performance;
} ibmuftian_driver;
extern ibmuftian_driver ibmuftian;

struct ibmuftian_fpga;

typedef struct ibmuftian_fd
{
    dev_info *dev;
    struct file * filep;
    unsigned int mln_no;    
} ibmuftian_fd;

#define WR_EXEC _IOW(IBMUFTIAN_MAGIC, 71, ioctl_inst)
#define WR_MEM  _IOW(IBMUFTIAN_MAGIC, 72, mem_buff)
#define RD_MEM  _IOW(IBMUFTIAN_MAGIC, 73, mem_buff)

ssize_t ibmuftian_version_show(struct kobject *, struct kobj_attribute *, char *);
int register_driver(struct ibmuftian_fpga *);
void unregister_driver(ibmuftian_fpga *);

#endif
