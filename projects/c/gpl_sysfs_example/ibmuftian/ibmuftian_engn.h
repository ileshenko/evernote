#ifndef _IBMUFTIAN_ENGN_H_
#define _IBMUFTIAN_ENGN_H_

#include <linux/dma-mapping.h>
#include <linux/list.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_pci_bar.h"
#include "ibmuftian_ioctl.h"

#define to_engn(x)                         container_of((x), ibmuftian_engn, kobj)
#define to_engn_attr(x)                    container_of((x), engn_attr, attr)

struct ibmuftian_fpga;



typedef struct ibmuftian_engn
{
    struct kobject kobj;
    ibmuftian_bar *bar0;
    ibmuftian_bar *bar4;
    struct ibmuftian_fpga *fpga;
    bd_book_keeping buff_desc;
    int id;
    int number;
    int aborted;
    unsigned short bd_id;
    spinlock_t id_lock;
    job_book_keeping jobs;
    struct list_head list;
    struct timeval duration;
    struct timeval engn_utilized;
} ibmuftian_engn;

typedef struct engn_attr
{
        struct attribute attr;
        ssize_t (*show)(ibmuftian_engn *eng, struct engn_attr *, char *);
        ssize_t (*store)(ibmuftian_engn *eng, struct engn_attr *, const char *, size_t);
} engn_attr;

ssize_t engn_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
int get_engn_count(void);
int create_engn(struct ibmuftian_fpga *, int);
void engn_delete(ibmuftian_engn *);
void reset_aborted_engn(ibmuftian_engn *);

#endif
