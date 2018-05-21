#ifndef _IBMUFTIAN_INTERRUPT_H_
#define _IBMUFTIAN_INTERRUPT_H_

#include "ibmuftian_utils.h"

struct ibmuftian_bar;

void clear_isr_status(struct ibmuftian_bar *, u32, u32);
irqreturn_t ibmuftian_isr(int, void *);
void obd_completion_tasklet(unsigned long);
void ibd_completion_tasklet(unsigned long);
void enable_isr( struct ibmuftian_bar *, u32, u32);
u32 get_isr_status(struct ibmuftian_bar *, u32, u32);

#endif

