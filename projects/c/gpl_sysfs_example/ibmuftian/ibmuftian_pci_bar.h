#ifndef _IBMUFTIAN_PCI_BAR_H_
#define _IBMUFTIAN_PCI_BAR_H_

#include <linux/kobject.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <asm/types.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_interrupt.h"

#define to_bar(x) container_of((x), ibmuftian_bar, kobj)
#define to_bar_attr(x) container_of(x, bar_attr, attr)

struct ibmuftian_engn;

typedef struct interrupt_regs
{
    u32 enable_reg;
    u32 status_reg;
} interrupt_regs;

typedef struct completion_interrupt_service
{
    struct tasklet_struct ibd_comp_tasklet;
    struct tasklet_struct obd_comp_tasklet;
    struct timer_list ibd_comp_timer;
    struct timer_list obd_comp_timer;
} completion_interrupt_service;

typedef union bar_cmd_type
{
    char cmd_type_str[CMD_TYPE_LEN];
    int cmd_type_int;
} bar_cmd_type;

typedef struct bar_command
{
    unsigned params[BAR_NUM_PARAMS];
    bar_cmd_type command_type;
} bar_command;

typedef struct ibmuftian_bar
{
    int num;
    struct kobject kobj;
    struct pci_dev *pdev;
    int engn_id;
    void __iomem *addr;
    interrupt_regs *register_list;
    completion_interrupt_service interrupt_service;
    unsigned long mem_size;
    bar_command command;
    spinlock_t bar_lock;
    char command_buff[BAR_COMMAND_LEN];
    char command_result[BAR_COMMAND_LEN];
} ibmuftian_bar;

typedef struct bar_attr
{
    struct attribute attr;
    ssize_t (*show) (ibmuftian_bar *, struct bar_attr *, char *);
    ssize_t (*store)(ibmuftian_bar *, struct bar_attr *, const char *, size_t);
} bar_attr;

u32 read_bar_addr(ibmuftian_bar *, u32);
u32 write_bar_addr(ibmuftian_bar *, u32, u32);
void bar_delete(ibmuftian_bar *);
ibmuftian_bar * create_bar(struct ibmuftian_engn *, struct pci_dev *, int, unsigned long, unsigned long, struct kset *);

#endif
