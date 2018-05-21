#include "ibmuftian_sysfs.h"
#include "ibmuftian.h"

static struct kobj_attribute utilization = __ATTR(utilization, S_IRUGO | S_IWUGO, utilization_show, utilization_init);
static struct kobj_attribute driver_version = __ATTR(driver_version, S_IRUGO, ibmuftian_version_show, NULL);

static struct attribute *ibmuftian_attrs[] =
{
    &utilization.attr,
    &driver_version.attr,
    NULL,
};

static struct attribute_group ibmuftian_attr_group =
{
    .attrs = ibmuftian_attrs,
};

static struct kobj_attribute board_count = __ATTR(board_count, S_IRUGO, board_count_show, NULL);
static struct kobj_attribute fpga_count = __ATTR(fpga_count, S_IRUGO, fpga_count_show, NULL);
static struct kobj_attribute engn_count = __ATTR(engn_count, S_IRUGO, engn_count_show, NULL);

/******************************PARENT DIRECTORIES******************************/
static int init_board(void)
{
    int err = SUCCESS;
    struct kobject *kobj;
    
    PRINT(DBG_PRAM({SYSFS_DBG}), "creaing the board sysfs");
                
    ibmuftian.board_c = kzalloc(sizeof(ibmuftian_board_container), GFP_KERNEL);    
    if(unlikely(!ibmuftian.board_c))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to allocate memory for board container");
        err = -ENOMEM;
        return err;
    }
    
    atomic_set(&ibmuftian.board_c->count, 0);
    
    INIT_LIST_HEAD(&(ibmuftian.board_c->board_l));
    
    ibmuftian.board_c->kobjset = kset_create_and_add("board", NULL, ibmuftian.kobj);
	if(unlikely(!ibmuftian.board_c->kobjset))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to create board kset");
        err = -EEXIST;
        goto exit_free;
    }

    kobj = kobject_get(&ibmuftian.board_c->kobjset->kobj);
    if (unlikely(sysfs_create_file(&ibmuftian.board_c->kobjset->kobj, &board_count.attr)))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "board sysfs create file failed");
        kobject_put(kobj);
        err = FAILURE;
        goto exit_free;
    }

    kobject_put(kobj);

    PRINT(DBG_PRAM({SYSFS_DBG}), "board sysfs created");
    
    return err;
    
exit_free:
    kfree(ibmuftian.board_c);
    return err;    
}

static void destroy_board(void)
{
    struct kobject *kobj;
    
    PRINT(DBG_PRAM({SYSFS_DBG}), "reomoving board sysfs");
        
    if (unlikely(!ibmuftian.board_c))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "board container NULL");
        return;
    }

    if (likely(ibmuftian.board_c->kobjset))
    {
        kobj = kobject_get(&ibmuftian.board_c->kobjset->kobj);
        sysfs_remove_file(&ibmuftian.board_c->kobjset->kobj, &board_count.attr);
        atomic_set(&ibmuftian.board_c->count,0);
        kobject_put(kobj);
    }
    
    kset_unregister(ibmuftian.board_c->kobjset);
    ZERO_AND_FREE(ibmuftian.board_c);
    ibmuftian.board_c = NULL;
}

static int init_fpga(void)
{
    int err = SUCCESS;
    struct kobject *kobj;
    
    PRINT(DBG_PRAM({SYSFS_DBG}), "creaing the fpga sysfs");
    
    ibmuftian.fpga_c = kzalloc(sizeof(ibmuftian_fpga_container), GFP_KERNEL);    
    if(unlikely(!ibmuftian.fpga_c))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to allocate memory for fpga container");
        err = -ENOMEM;
        return err;
    }
    
    atomic_set(&ibmuftian.fpga_c->count, 0);
    
    INIT_LIST_HEAD(&(ibmuftian.fpga_c->fpga_l));
	
    ibmuftian.fpga_c->kobjset = kset_create_and_add("fpga", NULL, ibmuftian.kobj);
	if(unlikely(!ibmuftian.fpga_c->kobjset))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to create fpga kset");
        err = -EEXIST;
        goto exit_free;
    }
    
    kobj = kobject_get(&ibmuftian.fpga_c->kobjset->kobj);
    if (unlikely(sysfs_create_file(&ibmuftian.fpga_c->kobjset->kobj, &fpga_count.attr)))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "fpga create sysfs failed");
        kobject_put(kobj);
        err = FAILURE;
        goto exit_free;
    }     
    kobject_put(kobj);

    
    PRINT(DBG_PRAM({SYSFS_DBG}), "fpga sysfs created");

    return err;
    
exit_free:
    kfree(ibmuftian.fpga_c);
    return err;
}

static void destroy_fpga(void)
{
    struct kobject *kobj;
    
    PRINT(DBG_PRAM({SYSFS_DBG}), "reomoving fpga sysfs");
    
    if (unlikely(!ibmuftian.fpga_c))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "fpga container NULL");
        return;
    }

    if (likely(ibmuftian.fpga_c->kobjset))
    {
        kobj = kobject_get(&ibmuftian.fpga_c->kobjset->kobj);
        sysfs_remove_file(&ibmuftian.fpga_c->kobjset->kobj, &board_count.attr);
        atomic_set(&ibmuftian.fpga_c->count,0);
        kobject_put(kobj);
    }
    
    kset_unregister(ibmuftian.fpga_c->kobjset);
    ZERO_AND_FREE(ibmuftian.fpga_c);
    ibmuftian.fpga_c = NULL;
}

static int init_engn(void)
{
    int err = SUCCESS;
    struct kobject *kobj;
    
    PRINT(DBG_PRAM({SYSFS_DBG}), "creaing the engn sysfs");
    
    ibmuftian.engn_c = kzalloc(sizeof(ibmuftian_engn_container), GFP_KERNEL);    
    if(unlikely(!ibmuftian.engn_c))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to allocate memory for engn container");
        err = -ENOMEM;
        return err;
    }
    
    atomic_set(&ibmuftian.engn_c->count, 0);
    
    INIT_LIST_HEAD(&(ibmuftian.engn_c->engn_l));
    
    ibmuftian.engn_c->kobjset = kset_create_and_add("engn", NULL, ibmuftian.kobj);
	if(unlikely(!ibmuftian.engn_c->kobjset))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to create engn kset");
        err = -EEXIST;
        goto exit_free;
    }
    
    kobj = kobject_get(&ibmuftian.engn_c->kobjset->kobj);
    if (unlikely(sysfs_create_file(&ibmuftian.engn_c->kobjset->kobj, &engn_count.attr)))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "engn create sysfs failed");
        kobject_put(kobj); 
        err = FAILURE;
        goto exit_free;
    }     
    kobject_put(kobj); 
            
    PRINT(DBG_PRAM({SYSFS_DBG}), "engn sysfs created");
    
    return err;
    
exit_free:
    kfree(ibmuftian.engn_c);
    return err;
}

static void destroy_engn(void)
{
    struct kobject *kobj;
        
    PRINT(DBG_PRAM({SYSFS_DBG}), "reomoving engn sysfs");

    if (unlikely(!ibmuftian.engn_c)) 
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "engn container NULL");
        return;
    }

    if (likely(ibmuftian.engn_c->kobjset))
    {
        kobj = kobject_get(&ibmuftian.engn_c->kobjset->kobj);
        sysfs_remove_file(&ibmuftian.engn_c->kobjset->kobj, &board_count.attr);
        atomic_set(&ibmuftian.engn_c->count,0);
        kobject_put(kobj);
    }
    
    kset_unregister(ibmuftian.engn_c->kobjset);
    ZERO_AND_FREE(ibmuftian.engn_c);
    ibmuftian.engn_c = NULL;
}

/********************************************************************************/

int initialize_driver(void)
{
    int err = SUCCESS;

    //CREATING THE DRIVER SYSFS DIRECTORY
    PRINT(DBG_PRAM({SYSFS_DBG}), "initalizing sysfs file system");
        
    ibmuftian.kobj = kobject_create_and_add(IBMUFTIAN_NAME, NULL);
    if (unlikely(!ibmuftian.kobj))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed kobj create and add");
        err = -ENOMEM;
        return err;
    }

    // Create the file associated with this kobject
    if (unlikely(sysfs_create_group(ibmuftian.kobj, &ibmuftian_attr_group)))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to create driver version sysfs file");
        err = -EEXIST;
        goto exit_clean_all;
    }
    
    //CREATING THE OTHER PARENT DIRECTORIES
    
    if(unlikely(err = init_board()))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to create board sysfs");
        goto exit_clean_sysfs; 
    }
    
    if(unlikely(err = init_fpga()))
    {
        
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to create fpga sysfs");
        goto exit_clean_board;
    }
    
    if(unlikely(err = init_engn()))
    {
        PRINT(DBG_PRAM({SYSFS_DBG, ERR_DBG}), "failed to create engn sysfs");
        goto exit_clean_fpga;
         
    }

    PRINT(DBG_PRAM({SYSFS_DBG}), "file system initalization done");
        
    return err;

exit_clean_all:
    destroy_engn();    
exit_clean_fpga:
    destroy_fpga();
exit_clean_board:
    destroy_board();
exit_clean_sysfs:
    kobject_put(ibmuftian.kobj);
    return err;    
}

void destroy_driver(void)
{

    PRINT(DBG_PRAM({SYSFS_DBG}), " removing sysfs directories");
    
    destroy_engn();
    destroy_fpga();
    destroy_board();

    if(likely(ibmuftian.kobj))
    {
        sysfs_remove_group(ibmuftian.kobj, &ibmuftian_attr_group);
        kobject_put(ibmuftian.kobj);
    }
}
