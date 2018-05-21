#ifndef _IBMUFTIAN_SYSFS_H_
#define _IBMUFTIAN_SYSFS_H_

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_pci.h"

int initialize_driver(void);
void destroy_driver(void);

#endif
