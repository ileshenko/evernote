#ifndef _IBMUFTIAN_UTILS_H_
#define _IBMUFTIAN_UTILS_H_

#include <linux/kernel.h>
#include <asm/string.h>
#include <linux/ctype.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/pagemap.h>
#include <linux/time.h>

#include "ibmuftian_constants.h"

struct ibmuftian_job;
struct sgt_container;

#define hi_addr(x) ((u32) ((sizeof (x) == 4 ? 0 : (x) >> 32)))
#define lo_addr(x) ((u32) (x))

#define BITS(hi_, lo_) (((hi_) - (lo_)) + 1)

#define set_page_reserved(p, c) { int i; for (i = 0; i < (c); i++) SetPageReserved(((p) + i)); }
#define clr_page_reserved(p, c) { int i; for (i = 0; i < (c); i++) ClearPageReserved(((p) + i)); }

#define safe_sprintf(buf, bufsz, fmt, ...) ({ int n_ = snprintf(buf, bufsz, fmt , ## __VA_ARGS__); n_ == -1 ? 0 : n_ < bufsz ? n_ : bufsz - 1; })

#define RANGE(hi, lo) (((hi) - (lo)) + 1)

#define PRESCALE(khz) (WISHBONE_FREQ / (5 * khz * 1000)) - 1

#define ZERO_AND_FREE(x) (memset(x, 0, sizeof(*x)),kfree(x))

#define bits_set(reg, bitmask) (((reg) & (bitmask)) == (bitmask))

#define ACK_INTERRUPT(x) write_bar_addr(x, GENERAL_MSI_ACK, ACK_VAL)

unsigned long __cstr2ulong(const char *);
int trim_str(char *);
int map_user_pages(struct sgt_container **, unsigned long , unsigned long , enum dma_data_direction , unsigned long , int* , unsigned int *,struct page ***, int *, void **);
void unmap_user_pages(struct sgt_container *, int , int , int , int , struct page **, int , void* );
dma_addr_t map_user_memory(struct pci_dev* , unsigned long , unsigned long , enum dma_data_direction direction, int *, struct page ***, int *, void **);
void unmap_user_memory(struct pci_dev *,dma_addr_t , enum dma_data_direction ,  int ,struct page **, int ,  void* , unsigned long  );
ssize_t utilization_show(struct kobject *, struct kobj_attribute *, char *);
ssize_t utilization_init(struct kobject *, struct kobj_attribute *, const char *, size_t);

#endif
