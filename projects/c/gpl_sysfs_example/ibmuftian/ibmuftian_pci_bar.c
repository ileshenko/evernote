#include "ibmuftian_pci_bar.h"
#include "ibmuftian.h"
#include "ibmuftian_engn.h"

interrupt_regs irq_bar4_table[]={
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},
	{0x0,0x0},//{0x000062C4,0x000060C4},
	{0x0,0x0},//{0x000062C8,0x000060C8},
	{0x0,0x0},//{0x000062CC,0x000060CC},
	{0x000062D0,0x000060D0},
	{0x0,0x0},//{0x000062D4,0x000060D4},
	{0x000062D8,0x000060D8},
	{0x0,0x0},
	{0x000062E0,0x000060E0},
	{0x000062E4,0x000060E4},
	{0x000062E8,0x000060E8},
	{0x000062EC,0x000060EC},
	{0x000062F0,0x000060F0},
	{0x000062F4,0x000060F4},
	{0x000062F8,0x000060F8},
	{0x0,0x0}
};

interrupt_regs irq_bar0_table[]={
	{0x00046280,0x00046080},
	{0x0,0x0},
	{0x00046288,0x00046088},
	{0x0,0x0},
	{0x00046290,0x00046090},
	{0x0,0x0},
	{0x00046298,0x00046098},
	{0x0,0x0},
	{0x000462A0,0x000460A0},
	{0x0,0x0},
	{0x000462A8,0x000460A8},
	{0x0,0x0},
	{0x000462B0,0x000460B0},
	{0x0,0x0},
	{0x000462B8,0x000460B8},
	{0x0,0x0},
	{0x000462C0,0x000460C0},
	{0x0,0x0},
	{0x000462C8,0x000460C8},
	{0x0,0x0},
	{0x000462D0,0x000460D0},
	{0x0,0x0},
	{0x000462D8,0x000460D8},
	{0x0,0x0},
	{0x000462E0,0x000460E0},
	{0x000462E4,0x000460E4},
	{0x000462E8,0x000460E8},
	{0x0,0x0},
	{0x000462F0,0x000460F0},
	{0x0,0x0},//{0x000462F4,0x000460F4},{0x000462F4,0x000460F4},	
	{0x000462F8,0x000460F8},
	{0x000462FC,0x000460FC}
};

void create_int_regs_entry(ibmuftian_bar *bar)
{
    if(bar->num)
        bar->register_list = irq_bar4_table;
    else
        bar->register_list = irq_bar0_table;
}

void ibd_completion_timer(unsigned long pBar)
{
    ibmuftian_engn *engn = (ibmuftian_engn *)pBar;
    tasklet_schedule(&engn->bar0->interrupt_service.ibd_comp_tasklet);
}

void obd_completion_timer(unsigned long pBar)
{
    ibmuftian_engn *engn = (ibmuftian_engn *)pBar;
    tasklet_schedule(&engn->bar0->interrupt_service.obd_comp_tasklet);
}

void init_completion_interrupt_services(ibmuftian_bar *bar, ibmuftian_engn * engn)
{
    setup_timer(&bar->interrupt_service.ibd_comp_timer, ibd_completion_timer, (unsigned long)engn);
    setup_timer(&bar->interrupt_service.obd_comp_timer, obd_completion_timer, (unsigned long)engn);
	tasklet_init(&bar->interrupt_service.ibd_comp_tasklet, ibd_completion_tasklet, (unsigned long)engn);	
	tasklet_init(&bar->interrupt_service.obd_comp_tasklet, obd_completion_tasklet, (unsigned long)engn);	
}

u32 read_bar_addr(ibmuftian_bar *bar, u32 addr)
{
    u32 mem_val = 0;
	void __iomem *address = NULL;
    if(likely(bar && bar->addr))
    {
        if(unlikely(addr % 4))
        {
            PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "addr %8.8x not 4 byte alligned", addr);
            return -EFAULT;
        }
        else
        {
            address = bar->addr + addr;
            mem_val = ioread32(address);
		    return mem_val;
        }
	}

    PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "failed to read data from addr %8.8x", addr);
    return -EINVAL;
	
}

u32 write_bar_addr(ibmuftian_bar *bar, u32 addr, u32 val)
{
    void __iomem * address = NULL;
    if(likely(bar && bar->addr))
    {
        if(unlikely(addr % 4))
        {
            PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "addr %8.8x not 4 byte alligned", addr);
            return -EFAULT;
        }
        else
        {
            address = bar->addr + addr;
            iowrite32(val, address);
            return SUCCESS;
        }
    }

    PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "failed to write data %8.8x to addr %8.8x", val, addr);
	return FAILURE;
}

static void __iomem * ibmuftian_iomap(struct pci_dev *dev, int bar, unsigned long start, unsigned long end)
{
    unsigned long _start = pci_resource_start(dev, bar);
    unsigned long _flags = pci_resource_flags(dev, bar);
    unsigned long _len   = pci_resource_len(dev, bar);

    _start += start;
    _len = min((end - start), _len);

    PRINT(DBG_PRAM({BAR_DBG}), "mapping s:0x%08lX l:0x%08lX", _start, _len);

    if (!_len || !_start)                                   return NULL;
    if (_flags & IORESOURCE_IO)                             return ioport_map(_start, _len);
    if (_flags & (IORESOURCE_MEM || IORESOURCE_CACHEABLE))  return ioremap(_start, _len);
    if (_flags & IORESOURCE_MEM)                            return ioremap_nocache(_start, _len);
    return NULL;
}

void execute_bar_command(ibmuftian_bar *bar)
{
    u32 read_buff = 0x0;
    u32 write_buff = 0x0;
    
    if(bar->command.command_type.cmd_type_int == BAR_READ)
    {
        read_buff = read_bar_addr(bar, bar->command.params[BAR_CMD_ADDR]);
        if(read_buff == -EFAULT)    
        {
            PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "Failed to read buff %8.8x", bar->command.params[BAR_CMD_ADDR]);
            safe_sprintf(bar->command_result, BAR_BUFF_LEN, "failed to read");
        }
        else
            safe_sprintf(bar->command_result, BAR_BUFF_LEN, "%8.8x", read_buff);
    }
    else
    {
        write_buff = bar->command.params[BAR_CMD_DATA];
        if(write_bar_addr(bar, bar->command.params[BAR_CMD_ADDR], bar->command.params[BAR_CMD_DATA]))
        {
            PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "Failed to write buff %8.8x", bar->command.params[BAR_CMD_ADDR]);
            safe_sprintf(bar->command_result, BAR_BUFF_LEN, "failed to write");
        }
    }
}

int validate_bar_cmd(ibmuftian_bar *bar)
{
    if(strcmp(bar->command.command_type.cmd_type_str, "\0"))
    {
        if(strcmp(bar->command.command_type.cmd_type_str, BAR_READ_CMD))
        {
            if(strcmp(bar->command.command_type.cmd_type_str, BAR_WRITE_CMD))
            {
                return FAILURE;
            }
            else
                bar->command.command_type.cmd_type_int = BAR_WRITE;
            
        }
        else
            bar->command.command_type.cmd_type_int = BAR_READ;
    }
    else
        return FAILURE;

    return SUCCESS;
}

void parse_bar_command(ibmuftian_bar *bar)
{
    char *cmd_ptr = bar->command_buff;
    char *endptr, *parms;
    int params;
    if(trim_str(cmd_ptr))
    {
        PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "Command string empty");
        safe_sprintf(bar->command_result,BAR_BUFF_LEN,"no command");
    }

    PRINT(DBG_PRAM({BAR_DBG}), "The command is %s", cmd_ptr);

    parms = strsep(&cmd_ptr, " ");
    if(parms != NULL) strncpy(bar->command.command_type.cmd_type_str, parms, CMD_TYPE_LEN);
    else strncpy(bar->command.command_type.cmd_type_str, "\0", CMD_TYPE_LEN);

    if(validate_bar_cmd(bar))
    {
        PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "Invalid bar command");
        safe_sprintf(bar->command_result,BAR_BUFF_LEN,"invalid bar command");
    }
    else
    {
        for(params = 0; params < (bar->command.command_type.cmd_type_int == BAR_READ ? 1 : 2) ; params ++)
        {
            parms = strsep(&cmd_ptr, " ");
            
            if(parms != NULL) bar->command.params[params] = simple_strtol(parms, &endptr, HEX);
            else bar->command.params[params] = 0;
        }

        execute_bar_command(bar);        
    }
    
}

static ssize_t bar_command_show(ibmuftian_bar *bar, bar_attr *attr, char *buf)
{
    return safe_sprintf(buf, PAGE_SIZE, "%s\n", bar->command_result);
}

static ssize_t bar_command_store(ibmuftian_bar *bar, bar_attr *attr, const char *buf, size_t count)
{
    char *cmd_buff = bar->command_buff;
    if(count > BAR_COMMAND_LEN)
    {
        PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "size of command exceeds max length");
        return 0;
    }

    strncpy(cmd_buff, buf, count);

    parse_bar_command(bar);
    return count;
}

static ssize_t attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    bar_attr * bar_attribute = to_bar_attr(attr);
    ibmuftian_bar * bar = to_bar(kobj);
    
    return bar_attribute->show ? bar_attribute->show(bar, bar_attribute, buf) : -EIO;
}

static ssize_t attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    bar_attr * bar_attribute = to_bar_attr(attr);
    ibmuftian_bar * bar = to_bar(kobj);
    
    return bar_attribute->store ? bar_attribute->store(bar, bar_attribute, buf, count) : -EIO;
}

static void release(struct kobject *kobj)
{
    ibmuftian_bar * bar = to_bar(kobj);

    if(unlikely(!bar)) return;
    
    tasklet_kill(&bar->interrupt_service.ibd_comp_tasklet);
    tasklet_kill(&bar->interrupt_service.obd_comp_tasklet);
    del_timer(&bar->interrupt_service.ibd_comp_timer);
    del_timer(&bar->interrupt_service.obd_comp_timer);

    pci_iounmap(bar->pdev, bar->addr);

    bar->addr = NULL;

    ZERO_AND_FREE(bar);
}

static ssize_t bar_show(struct ibmuftian_bar *bar, struct bar_attr *attr, char *buf)
{
    int num_chars = 0;

    if(strcmp(attr->attr.name, "mem_size") == 0)
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%lu\n", bar->mem_size);
    }
    else
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "method for '%s' not yet implemented\n", attr->attr.name);
    }

    return num_chars;
}

ibmuftian_bar * create_bar(struct ibmuftian_engn *engn, struct pci_dev *pdev, int bar, unsigned long start, unsigned long end, struct kset *parent)
{
    ibmuftian_bar * new_bar = NULL;
    char name[NAME_LEN];
    unsigned long start_addr, end_addr;

    static bar_attr mem_size = __ATTR(mem_size, S_IRUGO, bar_show, NULL);
    static bar_attr bar_command = __ATTR(bar_command, S_IRUGO | S_IWUGO, bar_command_show, bar_command_store);

    static struct attribute *bar_default_attrs[] = 
    {
        &mem_size.attr,
        &bar_command.attr,
        NULL,
    };

    static struct sysfs_ops bar_sysfs_ops = { .show = attr_show, .store = attr_store };
    
    static struct kobj_type bar_ktype =
    {
        .release        = release,
        .sysfs_ops      = &bar_sysfs_ops,
        .default_attrs  = bar_default_attrs,
    };
    
    PRINT(DBG_PRAM({BAR_DBG}), "creating bar%d", bar);

    PRINT(DBG_PRAM({BAR_DBG}), "allocating memory for bar%d", bar);

    new_bar = kzalloc(sizeof(ibmuftian_bar), GFP_KERNEL);
    if(unlikely(!new_bar))
    {
        PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "failed to allocate memory for bar%d", bar);
    }

    new_bar->num = bar;
    new_bar->pdev = pdev;
    new_bar->kobj.kset = parent;
    new_bar->engn_id = bar ? 0 : (end/BAR0_SIZE) - 1;
    new_bar->addr = ibmuftian_iomap(pdev, bar, start, end);
    spin_lock_init(&new_bar->bar_lock);
    create_int_regs_entry(new_bar);
    if (!bar) init_completion_interrupt_services(new_bar, engn);

    start_addr = pci_resource_start(pdev, bar);
    end_addr = pci_resource_end(pdev, bar);
    new_bar->mem_size = (start_addr == end_addr ? 0: (end_addr - start_addr +1));
	 
        
    bar ? safe_sprintf(name, NAME_LEN, "%d", bar) : safe_sprintf(name, NAME_LEN, "%d_%d", bar, new_bar->engn_id);
    
    if (unlikely(kobject_init_and_add(&new_bar->kobj, &bar_ktype, NULL, name)))
    {
        PRINT(DBG_PRAM({BAR_DBG, ERR_DBG}), "kobject init/add failed for %s", name);
        goto clean_exit;
    }

    return new_bar;

clean_exit:
    kobject_put(&new_bar->kobj);
    ZERO_AND_FREE(new_bar);
    return NULL;
}

void bar_delete(ibmuftian_bar *bar)
{
    if(likely(bar))
    {
        kobject_put(&bar->kobj);
    }
}
