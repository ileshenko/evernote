#include "ibmuftian_fpga.h"
#include "ibmuftian.h"
#include "ibmuftian_pci.h"
#include "ibmuftian_containers.h"

struct kset* get_fpga_container(void)
{
    return likely(ibmuftian.fpga_c) ? ibmuftian.fpga_c->kobjset : NULL;
}

int get_fpga_count(void)
{
    return likely(ibmuftian.fpga_c)? atomic_read(&(ibmuftian.fpga_c->count)): 0;
}

ssize_t fpga_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_fpga_count());
}

void add_fpga(void)
{
    atomic_add(1, &(ibmuftian.fpga_c->count));
}

void remove_fpga(void)
{
    atomic_dec(&(ibmuftian.fpga_c->count));
}

void remove_fpga_l(int id)
{
    ibmuftian_fpga *fpga_ptr= NULL, *n_ptr = NULL;
    
    list_for_each_entry_safe(fpga_ptr, n_ptr, &(ibmuftian.fpga_c->fpga_l), list)
    {
        if(fpga_ptr->id == id)
        {
            list_del(&fpga_ptr->list);
            break;
        }
    }
}

ibmuftian_fpga * get_fpga_by_id(int id)
{
    ibmuftian_fpga *fpga_ptr = NULL;

    list_for_each_entry(fpga_ptr, &(ibmuftian.fpga_c->fpga_l), list)
    {
        if(fpga_ptr->id == id)
        {
            return fpga_ptr;
        }
    }

    return NULL;
}

static void release(struct kobject *kobj)
{
    ibmuftian_fpga *fpga = to_fpga(kobj);
    if (likely(fpga))
    {
        remove_fpga();
        ZERO_AND_FREE(fpga);
    }
}

static ssize_t attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    fpga_attr *fpga_attribute = to_fpga_attr(attr);
    ibmuftian_fpga *fpga = to_fpga(kobj);

    return fpga_attribute->show ? fpga_attribute->show(fpga, fpga_attribute, buf) : -EIO;
}

static ssize_t attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    fpga_attr *fpga_attribute = to_fpga_attr(attr);
    ibmuftian_fpga *fpga = to_fpga(kobj);

    return fpga_attribute->store ? fpga_attribute->store(fpga, fpga_attribute, buf, count) : -EIO;
}

static ssize_t fpga_show(ibmuftian_fpga *fpga, fpga_attr *attr, char *buf)
{
    int num_chars = 0;

    if (unlikely(!fpga) || unlikely(!buf) || unlikely(!attr)) return 0;

    if (!strcmp(attr->attr.name, "engn_count"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%u\n", fpga->engns.engn_count);
    }
    else if (!strcmp(attr->attr.name, "uta_available"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%u\n", fpga->uta_available);
    }
    else if (!strcmp(attr->attr.name, "irq_name"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%s\n", fpga->irq_dev_name);
    }
    else if (!strcmp(attr->attr.name, "irq"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%u\n", fpga->device_info.pdev->irq);
    }
    else if (!strcmp(attr->attr.name, "version"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%06X.%02x\n", fpga->fpga_rev_info.bit.date, fpga->fpga_rev_info.bit.revision);
    }
    else
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "method for '%s' not yet implemented\n", attr->attr.name);
    }
    
    return num_chars;
}

int create_fpga(ibmuftian_board *board, struct pci_dev *pdev)
{
    ibmuftian_fpga *new_fpga = NULL;
    int engn = 0, loop_var = 0, res = 0;
    u32 status_val = 0;
    int fpga_count = atomic_read(&(ibmuftian.fpga_c->count));

    static fpga_attr irq = __ATTR(irq, S_IRUGO, fpga_show, NULL);
    static fpga_attr irq_name = __ATTR(irq_name, S_IRUGO, fpga_show, NULL);
    static fpga_attr engn_count = __ATTR(engn_count, S_IRUGO, fpga_show, NULL);
    static fpga_attr uta_available = __ATTR(uta_available, S_IRUGO, fpga_show, NULL);
    static fpga_attr version = __ATTR(version, S_IRUGO, fpga_show, NULL);
    
    static struct attribute *fpga_attrs[] =
    {
        &irq.attr,
        &irq_name.attr,
        &engn_count.attr,
        &uta_available.attr,
        &version.attr,
        NULL,
    };
    
    static struct sysfs_ops fpga_sysfs_ops = { .show  = attr_show, .store = attr_store };
    
    static struct kobj_type fpga_ktype =
    {
        .release = release,
        .sysfs_ops = &fpga_sysfs_ops,
        .default_attrs = fpga_attrs,
    };
    
    PRINT(DBG_PRAM({FPGA_DBG}), "creating fpga");

    PRINT(DBG_PRAM({FPGA_DBG}), "allocating space for fpga");

    
    new_fpga = kzalloc(sizeof(ibmuftian_fpga), GFP_KERNEL);
    list_add(&new_fpga->list, &(ibmuftian.fpga_c->fpga_l));
        
    if (unlikely(!new_fpga))
    {
        PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "failed to allocate space for fpga");
        return FAILURE;
    }
    
    new_fpga->device_info.pdev = pdev;
    new_fpga->device_info.fpga = new_fpga;
    new_fpga->kobj.kset = get_fpga_container();
    new_fpga->id = fpga_count;
    new_fpga->uta_available = 1;

    board->fpga = new_fpga;
    add_fpga();

    PRINT(DBG_PRAM({FPGA_DBG}), "fpga added initializing sysfs");
    
    if (unlikely(kobject_init_and_add(&new_fpga->kobj, &fpga_ktype, NULL, "%d", new_fpga->id)))
    {
        PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}),"kobject init/add failed");
        goto error_exit1;
    }

    new_fpga->bar.bar_kset = kset_create_and_add("bar", NULL, &new_fpga->kobj);
    if (unlikely(!new_fpga->bar.bar_kset))
    {
        PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "Failed to create fpga bar object container");
        goto error_exit2;
    }

    PRINT(DBG_PRAM({FPGA_DBG}), "sysfs created");
    
    safe_sprintf(new_fpga->irq_dev_name, sizeof new_fpga->irq_dev_name, "%s:fpga%d", IBMUFTIAN_NAME, new_fpga->id);
    if (ibmuftian_pci_init(pdev, new_fpga->irq_dev_name, &new_fpga->link_width, (void *)new_fpga))
    {
        PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "Failed to setup PCIe device");
        goto error_exit3;
    }

    PRINT(DBG_PRAM({FPGA_DBG}), "successfully initialized PCI device");
    
    new_fpga->bar.bar4 = create_bar(NULL, pdev, 4, 0, BAR4_SIZE, new_fpga->bar.bar_kset);
    if (!new_fpga->bar.bar4)
    {
        PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "Error creating bar4");
        goto error_exit4;
    }
    board->bar4 = new_fpga->bar.bar4;

    PRINT(DBG_PRAM({FPGA_DBG}), "bar4 created");

    for(loop_var = 0; loop_var < GEN_INT_REGS_LEN; loop_var++)
    {
        status_val = 0;
        if(new_fpga->bar.bar4->register_list[loop_var].status_reg)
        {
            if((status_val = get_isr_status(new_fpga->bar.bar4, loop_var, ALL_IRQ)))
                clear_isr_status(new_fpga->bar.bar4, loop_var, ALL_IRQ);
        }
    }
    
    for(loop_var = 0; loop_var < GEN_INT_REGS_LEN; loop_var++)
    {
        if(new_fpga->bar.bar4->register_list[loop_var].enable_reg)
            write_bar_addr(new_fpga->bar.bar4, new_fpga->bar.bar4->register_list[loop_var].enable_reg, ENABLE_ALL);
    }
    
    new_fpga->fpga_rev_info.raw = read_bar_addr(new_fpga->bar.bar4, FPGA_INFO_ADDR);
    new_fpga->fpga_info.raw = read_bar_addr(new_fpga->bar.bar4, FPGA_ENGN_INFO_ADDR);

    if(new_fpga->fpga_info.bit.board_type == FIN)
    {
        board->board_type = FIN;

        PRINT(DBG_PRAM({IBMUFTIAN_DBG}), "FIN CARD");

        new_fpga->bar.bar2 = create_bar(NULL, pdev, 2, 0, FIN_BAR2_SIZE, new_fpga->bar.bar_kset); 
        if (!new_fpga->bar.bar2)
        {
            PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "Error creating bar2");
            goto error_exit5;
        }

        PRINT(DBG_PRAM({FPGA_DBG}), "bar2 created");
    }
    else if(new_fpga->fpga_info.bit.board_type == WRASSE)
    {
        board->board_type = WRASSE;

        PRINT(DBG_PRAM({IBMUFTIAN_DBG}), "WRASSE CARD");

        new_fpga->bar.bar2 = create_bar(NULL, pdev, 2, 0, WRASSE_BAR2_SIZE, new_fpga->bar.bar_kset); 
        if (!new_fpga->bar.bar2)
        {
            PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "Error creating bar2");
            goto error_exit5;
        }

        PRINT(DBG_PRAM({FPGA_DBG}), "bar2 created");
    }

    PRINT(DBG_PRAM({FPGA_DBG}), "read fgpa and engn info rev id is %d%d no. of engn %d", new_fpga->fpga_rev_info.bit.revision, new_fpga->fpga_rev_info.bit.date, new_fpga->fpga_info.bit.numberofengns);

    new_fpga->engns.engn_count = new_fpga->fpga_info.bit.numberofengns;
    
    new_fpga->i2c_cntrl = board->i2c_cntrl;
    if(create_flash(new_fpga))
    {
       PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "Error creating flash"); 
       goto error_exit6;
    }   

    new_fpga->engns.engines = kzalloc(sizeof(ibmuftian_engn *) * new_fpga->engns.engn_count, GFP_KERNEL);
	
	for (engn = 0; engn < new_fpga->engns.engn_count; engn++)
    {
        if(create_engn(new_fpga, engn))
        {
            PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "failed to create engn %d", engn);
        }
        PRINT(DBG_PRAM({FPGA_DBG}), "engn %d created", engn);

        for(loop_var = 0; loop_var < ENGN_INT_REGS_LEN; loop_var++)
        {
            if(new_fpga->engns.engines[engn]->bar0->register_list[loop_var].status_reg)
            {
                status_val = 0;
                if((status_val = get_isr_status(new_fpga->engns.engines[engn]->bar0, loop_var, ALL_IRQ)))
                    clear_isr_status(new_fpga->engns.engines[engn]->bar0, loop_var, status_val);
            }
        }
        
        for(loop_var = 0; loop_var < ENGN_INT_REGS_LEN; loop_var++)
        {
            if(new_fpga->engns.engines[engn]->bar0->register_list[loop_var].enable_reg)
            {
                write_bar_addr(new_fpga->engns.engines[engn]->bar0, new_fpga->engns.engines[engn]->bar0->register_list[loop_var].enable_reg, ENABLE_ALL);
            }
        }
         
        
    }

#ifdef MLN_ENVIRONMENT	
    for(loop_var = 0; loop_var < 4; loop_var++)
        atomic_set(new_fpga->uta_available_atomic + loop_var, 1);
#else
        atomic_set(&new_fpga->uta_available_atomic[0],1);
#endif 

    PRINT(DBG_PRAM({FPGA_DBG}), "registering IRQs");

    if ((res = request_irq( pdev->irq, ibmuftian_isr, IRQF_SHARED, new_fpga->irq_dev_name, (void*)new_fpga)) < 0)
    {
		dev_err(&pdev->dev, "Failed to install isr: %d\n", pdev->irq);
        PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "Failed to install isr: %d\n", pdev->irq);
		goto error_exit7;
	}

    PRINT(DBG_PRAM({FPGA_DBG}), "registering driver");
    
    if(register_driver(new_fpga))
    {
        PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "error registering driver");
        goto exit_clean_all;
    }
   
    synchronize_irq(new_fpga->device_info.pdev->irq);    
 
    return SUCCESS;
exit_clean_all:
    free_irq(pdev->irq, new_fpga);
error_exit7:
    for (engn = 0; engn < new_fpga->engns.engn_count; engn++)
    {
        engn_delete(new_fpga->engns.engines[engn]);
    }
    
    ZERO_AND_FREE(new_fpga->engns.engines);
    flash_delete(new_fpga->flash);
error_exit6:
    bar_delete(new_fpga->bar.bar2);
error_exit5:
    bar_delete(new_fpga->bar.bar4);
error_exit4:
    ibmuftian_pci_destroy(pdev, (void*)new_fpga);
error_exit3:
    kset_unregister(new_fpga->bar.bar_kset);
error_exit2:
    kobject_put(&new_fpga->kobj);
error_exit1:
    remove_fpga();
    list_del(&new_fpga->list);
    ZERO_AND_FREE(new_fpga);
    PRINT(DBG_PRAM({FPGA_DBG, ERR_DBG}), "Fpga error area");
    return FAILURE;
    
}

void fpga_delete(ibmuftian_fpga * fpga)
{
    int engn; 
    unregister_driver(fpga);
    PRINT(DBG_PRAM({FPGA_DBG}), "removing fpga %d", fpga->id);
    for (engn = (fpga->engns.engn_count - 1); engn >= 0; engn--)
    {
        PRINT(DBG_PRAM({ENGN_DBG}), "removing engn from fpga %d", engn);
        engn_delete(fpga->engns.engines[engn]);
    }

    ZERO_AND_FREE(fpga->engns.engines);
    synchronize_irq(fpga->device_info.pdev->irq);
    flash_delete(fpga->flash);
    ibmuftian_pci_destroy(fpga->device_info.pdev, (void*)fpga);
    bar_delete(fpga->bar.bar2);
    bar_delete(fpga->bar.bar4);
    kset_unregister(fpga->bar.bar_kset);
    remove_fpga_l(fpga->id);
    kobject_put(&fpga->kobj);
}
