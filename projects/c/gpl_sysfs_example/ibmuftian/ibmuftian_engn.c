#include "ibmuftian_engn.h"
#include "ibmuftian.h"
#include "ibmuftian_containers.h"


static ssize_t attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    engn_attr *engn_attribute = to_engn_attr(attr);
    ibmuftian_engn *engn = to_engn(kobj);

    return engn_attribute->show ? engn_attribute->show(engn, engn_attribute, buf) : -EIO;
}

static ssize_t attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    engn_attr *engn_attribute = to_engn_attr(attr);
    ibmuftian_engn *engn = to_engn(kobj);

    return engn_attribute->store ? engn_attribute->store(engn, engn_attribute, buf, count) : -EIO;
}

static ssize_t engn_show(ibmuftian_engn *engn, struct engn_attr *attr, char *buf)
{
    int num_chars= 0;

    if (unlikely(!engn) || unlikely(!buf) || unlikely(!attr)) return 0;

    if (strcmp(attr->attr.name, "id") == 0)  
    { 
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%d\n", engn->id); 
    }
    else                                            
    { 
        num_chars= safe_sprintf(buf, PAGE_SIZE, "method for '%s' not yet implemented\n", attr->attr.name); 
    }

    return num_chars;
}

void add_engn(void)
{
    atomic_add(1, &(ibmuftian.engn_c->count));
}

void remove_engn(void)
{
    atomic_dec(&(ibmuftian.engn_c->count));
}

void remove_engn_l(int number)
{
    ibmuftian_engn *engn_ptr = NULL, *n_ptr = NULL;
    
    list_for_each_entry_safe(engn_ptr, n_ptr, &(ibmuftian.engn_c->engn_l), list)
    {
        if(engn_ptr->number == number)
        {
            engn_ptr->fpga->engns.engines[engn_ptr->id] = NULL;
            list_del(&engn_ptr->list);
            break;
        }
    }
}

static void release(struct kobject *kobj)
{
    ibmuftian_engn *engn = to_engn(kobj);
    if (likely(engn))
    {
        remove_engn();
        ZERO_AND_FREE(engn);
    }
}

void reset_aborted_engn(ibmuftian_engn * engn)
{
    int loop_var = 0;
    u32 status_val = 0;

    reset_fifo_status_buffer(engn, IBD);
    reset_fifo_status_buffer(engn, OBD);

    for(loop_var = 0; loop_var < ENGN_INT_REGS_LEN; loop_var++)
    {
        if(engn->bar0->register_list[loop_var].status_reg)
        {
            status_val = 0;
            if((status_val = get_isr_status(engn->bar0, loop_var, ALL_IRQ)))
                clear_isr_status(engn->bar0, loop_var, status_val);
        }
    }

    for(loop_var = 0; loop_var < ENGN_INT_REGS_LEN; loop_var++)
    {
        if(engn->bar0->register_list[loop_var].enable_reg)
        {
            write_bar_addr(engn->bar0, engn->bar0->register_list[loop_var].enable_reg, ENABLE_ALL);
        }
    }
}

struct kset* get_engn_container(void)
{
    return likely(ibmuftian.engn_c) ? ibmuftian.engn_c->kobjset : NULL;
}

int get_engn_count(void)
{
    return likely(ibmuftian.engn_c)? atomic_read(&(ibmuftian.engn_c->count)): 0;
}

ssize_t engn_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_engn_count());
}

int create_engn(ibmuftian_fpga *fpga, int engn_id)
{
    ibmuftian_engn *new_engn = NULL;
    int engn_count = atomic_read(&(ibmuftian.engn_c->count));
    
    static engn_attr id = __ATTR(id, S_IRUGO, engn_show, NULL);

    static struct attribute *engn_default_attrs[] =
    {
        &id.attr,
        NULL,
    };

    static struct sysfs_ops engn_sysfs_ops = { .show = attr_show, .store = attr_store };

    static struct kobj_type engn_ktype =
    {
        .release        = release,
        .sysfs_ops      = &engn_sysfs_ops,
        .default_attrs  = engn_default_attrs,
    };
    
    PRINT(DBG_PRAM({ENGN_DBG}), "creating engn %d", engn_id);

    PRINT(DBG_PRAM({ENGN_DBG}), "allocating memory for engn %d", engn_id);

    
    new_engn = kzalloc(sizeof(ibmuftian_engn), GFP_KERNEL);
    if(unlikely(!new_engn))
    {
        PRINT(DBG_PRAM({ENGN_DBG, ERR_DBG}), "failed to allocate memory for engn %d", engn_id);
        return FAILURE;
    }

    (fpga->engns.engines)[engn_id] = new_engn;

    list_add(&new_engn->list, &(ibmuftian.engn_c->engn_l));
        
    add_engn();
    PRINT(DBG_PRAM({ENGN_DBG}), "engn count is %d", engn_count);
    new_engn->number = engn_count;
    new_engn->kobj.kset = get_engn_container();
    new_engn->id = engn_id;
    new_engn->bd_id = ZERO;
    new_engn->aborted = ZERO;
    new_engn->duration.tv_sec = ZERO;
    new_engn->duration.tv_usec = ZERO;
    spin_lock_init(&new_engn->id_lock);
    new_engn->bar0 = create_bar(new_engn, fpga->device_info.pdev, 0, BAR0_SIZE * engn_id, (BAR0_SIZE * engn_id) + BAR0_SIZE, fpga->bar.bar_kset);

    PRINT(DBG_PRAM({ENGN_DBG}), "bar 0 created for engn %d", engn_id);

    new_engn->bar4 = fpga->bar.bar4;
    new_engn->fpga = fpga;

    INIT_LIST_HEAD(&(new_engn->jobs.job_queue));
    spin_lock_init(&new_engn->jobs.job_queue_lock);

    if(init_bd_book_keeping(new_engn, IBD))
    {
        PRINT(DBG_PRAM({ENGN_DBG, ERR_DBG}), "buffer descriptor book keeping init failed for IBD");
        goto error_exit1;
    }

    if(init_bd_book_keeping(new_engn, OBD))
    {
        PRINT(DBG_PRAM({ENGN_DBG, ERR_DBG}), "buffer descriptor book keeping init failed for OBD");
        goto error_exit2;
    }
       
    if (unlikely(kobject_init_and_add(&new_engn->kobj, &engn_ktype, NULL, "%d_%d", fpga->id, engn_id)))
    {
        
        PRINT(DBG_PRAM({ENGN_DBG, ERR_DBG}), "kobject init/add failed for engn %d", engn_id);
        goto clean_exit;
    }

    return SUCCESS;

clean_exit:
    delete_bd_book_keeping(new_engn, OBD);
error_exit2:
    delete_bd_book_keeping(new_engn, IBD);
error_exit1:
    bar_delete(new_engn->bar0);
    kobject_put(&new_engn->kobj);
    remove_engn();
    list_del(&new_engn->list);
    (fpga->engns.engines)[engn_id] = NULL;
    ZERO_AND_FREE(new_engn);

    return FAILURE;    
}

void engn_delete(ibmuftian_engn *engn)
{
    if(likely(engn))
    {
        PRINT(DBG_PRAM({ENGN_DBG}), "removing engn %d", engn->number);
        delete_bd_book_keeping(engn, OBD);
        delete_bd_book_keeping(engn, IBD);
        bar_delete(engn->bar0);
        remove_engn_l(engn->number);
        kobject_put(&engn->kobj);
    }
}
