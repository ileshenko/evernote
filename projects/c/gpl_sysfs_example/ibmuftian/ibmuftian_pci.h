#ifndef _IBMUFTIAN_PCI_H_
#define _IBMUFTIAN_PCI_H_
#include <linux/pci.h>
#include <linux/pci_regs.h>
#include <linux/interrupt.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_hmc.h"

int ibmuftian_pci_register(void);
void ibmuftian_pci_unregister(void);
void ibmuftian_pci_destroy(struct pci_dev *, void *);
int ibmuftian_pci_init(struct pci_dev *, const char *, int *, void *);

#endif
