THIS_DIR := $(realpath $(dir $(shell readlink -f $(lastword $(MAKEFILE_LIST)))))

obj-m := ibmuftian.o

obj-m += sysmon.o

ccflags-y := -I.

sysmon-objs := sysmon_driver.o

ibmuftian-objs := ibmuftian_main.o\
	ibmuftian_pci.o\
	ibmuftian_board.o\
	ibmuftian_fpga.o\
	ibmuftian_pci_bar.o\
	ibmuftian_engn.o\
	ibmuftian_utils.o\
	ibmuftian_flash.o\
	ibmuftian_i2c.o\
	ibmuftian_sysfs.o\
	ibmuftian_ioctl.o\
	ibmuftian_hmc.o\
	ibmuftian_bdmgr.o\
	ibmuftian_interrupt.o
#linux := `uname -r`
#arch = $(shell uname -i)
ifdef UTA_MLN_ENABLE
	ccflags-y += -DMLN_ENVIRONMENT
endif

.PHONY: ibmuftian clean install 

ibmuftian: 
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(THIS_DIR) modules

clean:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(THIS_DIR) clean
	@rm -f /dev/ibmuftian

install:
	@./driver_load.sh ibmuftian.ko "$(DEBUG)"

uninstall:
	rmmod ibmuftian

flash:
	./fpga_flash.sh ibmuftian "$(UTA_DISABLE_FLASH)" "$(UTA_FORCE_FLASH)"
