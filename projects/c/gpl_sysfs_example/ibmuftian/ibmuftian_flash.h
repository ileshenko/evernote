#ifndef _IBMUFTIAN_FLASH_H_
#define _IBMUFTIAN_FLASH_H_

#include <linux/device.h>
#include <linux/unistd.h> 
#include <linux/spinlock.h>
#include <linux/semaphore.h>

#include "ibmuftian_pci_bar.h"
#include "ibmuftian_utils.h"

#define to_flash(x)                        container_of((x), ibmuftian_flash, kobj)
#define to_flash_attr(x)                   container_of((x), flash_attr, attr)

struct ibmuftian_fpga;

typedef struct dma_buf
{
    size_t bytes_read;
    size_t prog_offset;
    size_t sector_offset;
    void *prog_buf;
    dma_addr_t prog_dma_addr;
    void *verify_buf;
    dma_addr_t verify_dma_addr;
} dma_buf;

typedef enum 
{
    NO_OPERATION,
    ERASING,
    PROGRAMING,
    VERIFYING,
    SUCCEEDED,
    FAILED,
    UNKNOWN
} prog_states;

typedef enum
{
    NO_CODE,
    NO_SIZE,
    FLASH_LOCKED,
    FILE2BIG,
    FAIL_PRO_PG,
    FAIL_ER_PG,
    FAIL_VR_PG,
    IE,
    UNKNOWN_CODE
} fail_codes;

typedef struct ibmuftian_flash
{
    struct kobject kobj;
    unsigned id;
    u32 prog_size;
    spinlock_t flashing;
    int flash_in_progress;
    struct semaphore prog_sync;
    prog_states prog_state;
    fail_codes fail_code;
    u32 image_code;
    unsigned flash_pct;
    ibmuftian_bar *bar4;
    dma_buf dma;
} ibmuftian_flash;

typedef struct flash_attr
{
    struct attribute attr;
    ssize_t (*show) (ibmuftian_flash *, struct flash_attr *, char *);
    ssize_t (*store)(ibmuftian_flash *, struct flash_attr *, const char *, size_t);
} flash_attr;

void flash_delete(ibmuftian_flash *);
int create_flash(struct ibmuftian_fpga *);

#endif
