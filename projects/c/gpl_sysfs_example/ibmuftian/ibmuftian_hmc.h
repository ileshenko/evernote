#ifndef _IBMUFTIAN_HMC_H_
#define _IBMUFTIAN_HMC_H_
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/time.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_containers.h"

int trainHMC(ibmuftian_bar *, ibmuftian_i2c *, int);

#endif
