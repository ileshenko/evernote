#ifndef _IBMUFTIAN_FPGA_H_
#define _IBMUFTIAN_FPGA_H_

#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/time.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_engn.h"
#include "ibmuftian_i2c.h"
#include "ibmuftian_flash.h"

#define to_fpga(x)                         container_of((x), ibmuftian_fpga, kobj)
#define to_fpga_attr(x)                    container_of((x), fpga_attr, attr)

struct ibmuftian_board;

typedef struct ibmuftian_fpga_info
{
    union
    {
        u32 raw;
        struct
        {
            u32 numberofengns   : NUMBER_OF_ENGNS;           // [5:0]   Filter Quantity Number of filters in the chip
            u32 reserved        : RESERVED_ENGN_INFO_LEN;    // [7:6]   reserved
            u32 board_type      : BOARD_TYPE_LEN;            // [11:8]  board version
            u32 revision        : REVISION_LEN;              // [19:12]   revision number of engine
            u32 version         : VERSION_LEN;               // [23:20]   version number
            u32 product         : PRODUCT_LEN;               // [31:24]   Product
        } bit;
    };
} __attribute__ ((packed)) ibmuftian_fpga_info;

typedef struct ibmuftian_fpga_rev_info
{
    union
    {
        u32 raw;
        struct
        {
            u32 revision     : REVISION_LEN;    // [7:0]   revision number of day
            u32 date         : DATE_LEN;        // [31:8]   month-day-year
        } bit;
    };
} __attribute__ ((packed)) ibmuftian_fpga_rev_info;

typedef struct fpga_engn
{
    int engn_count;
    struct ibmuftian_engn **engines;
    atomic_t engn_inuse;
    int start_id;
    int end_id;
} fpga_engn;

typedef struct fpga_bar
{
    struct kset *bar_kset;
    ibmuftian_bar *bar2;
    ibmuftian_bar *bar4;    
} fpga_bar;

typedef struct dev_info
{
    struct pci_dev  *pdev;
    struct cdev dev_cdev;
    struct ibmuftian_fpga *fpga;
    dev_t dev;
} dev_info;

typedef struct ibmuftian_fpga
{
    struct kobject kobj;
    dev_info device_info;
    fpga_engn engns;
    int id;
    atomic_t open_fd;
    fpga_bar bar;
    ibmuftian_flash *flash;
    ibmuftian_i2c *i2c_cntrl;
    ibmuftian_fpga_rev_info fpga_rev_info;
    ibmuftian_fpga_info fpga_info;
    int link_width;
    int uta_available;
    atomic_t uta_available_atomic[MLN_NO];
    char irq_dev_name[NAME_LEN];
    struct list_head list;
} ibmuftian_fpga;

typedef struct fpga_attr
{
    struct attribute attr;
    ssize_t (*show) (ibmuftian_fpga *, struct fpga_attr *, char *);
    ssize_t (*store)(ibmuftian_fpga *, struct fpga_attr *, const char *, size_t);
} fpga_attr;

ssize_t fpga_count_show(struct kobject *, struct kobj_attribute *, char *);
int get_fpga_count(void);
int create_fpga(struct ibmuftian_board *, struct pci_dev *);
void fpga_delete(ibmuftian_fpga * );
ibmuftian_fpga * get_fpga_by_id(int);


#endif
