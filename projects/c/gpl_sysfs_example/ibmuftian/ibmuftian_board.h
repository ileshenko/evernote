#ifndef _IBMUFTIAN_BOARD_H_
#define _IBMUFTIAN_BOARD_H_

#include <linux/sysfs.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_fpga.h"

#define to_board(x)                        container_of((x), ibmuftian_board, kobj)
#define to_board_attr(x)                   container_of((x), board_attr, attr)

typedef struct vpd
{
    void *vpd_buf;
    char serno[VPD_SERNO_SIZE];
    char hwrev[VPD_HWREV_SIZE];
    char desc[VPD_DESC_SIZE];
} vpd;

typedef struct ibmuftian_board
{
    struct kobject kobj;
    int id;
    char board_name[BOARD_NAME_LEN];
    ibmuftian_fpga * fpga;
    ibmuftian_i2c *i2c_cntrl;
    vpd vpd_info;
    ibmuftian_bar *bar4;
    struct list_head list;
    int board_type;
} ibmuftian_board;

typedef struct board_attr
{
    struct attribute attr;
    ssize_t (*show) (ibmuftian_board *, struct board_attr *, char *);
    ssize_t (*store)(ibmuftian_board *, struct board_attr *, const char *, size_t);
} board_attr;

ssize_t board_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
int get_board_count(void);
int create_board(struct pci_dev*);
void board_delete(ibmuftian_board *);

#endif
