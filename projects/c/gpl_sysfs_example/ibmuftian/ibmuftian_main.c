#include "ibmuftian.h"
#include "ibmuftian_fpga.h"

ibmuftian_driver ibmuftian = {0};
int __read_mostly debug_areas[TOTAL_DBG] = {0};

int ibmuftian_open(struct inode *, struct file *);
int ibmuftian_close(struct inode *, struct file *);
static long ibmuftian_ioctl(struct file *, unsigned int, unsigned long);

struct file_operations ibmuftian_fops =
{
    .owner          = THIS_MODULE,
    .open           = ibmuftian_open,
    .release        = ibmuftian_close,
    .unlocked_ioctl = ibmuftian_ioctl,
};

void init_function(void(*engn_flip)(int,int))
{
    ibmuftian.functions.engn_inuse = engn_flip;
}

int dec_fd(ibmuftian_fpga *fpga)
{
#ifdef MLN_ENVIRONMENT
    int i,j;
    
    for (i = 0;i < MLN_NO; i++)
    {
        if(!atomic_dec_and_test(&fpga->uta_available_atomic[i]))
        {
            atomic_inc(&fpga->uta_available_atomic[i]);
        }
        else 
        {
            for (j = 0; j < MLN_NO; j++)
            {
                if(atomic_dec_and_test(&fpga->uta_available_atomic[j]))
                {
                    atomic_inc(&fpga->uta_available_atomic[j]);
                    fpga->uta_available = j + 1;
                    break;
                }
                else
                    atomic_inc(&fpga->uta_available_atomic[j]);
            }
            if(j >= MLN_NO)
                fpga->uta_available = 0;
            return (i + 1);
        }
    }
    
    return FAILURE;
#else
    fpga->uta_available = 0;
    if(!atomic_dec_and_test(&fpga->uta_available_atomic[0])) //atomic_dec_and_test() returns true if newval == 0
    {
        atomic_inc(&fpga->uta_available_atomic[0]);
        return FAILURE;
    }

    return SUCCESS;
#endif
}

int inc_fd(ibmuftian_fpga *fpga, int mln_no)
{
#ifdef MLN_ENVIRONMENT
    int i;
    atomic_inc(&fpga->uta_available_atomic[mln_no - 1]);
    
    for (i = 0; i < 4; i++)
    {
        if(atomic_dec_and_test(&fpga->uta_available_atomic[i]))
        {
            atomic_inc(&fpga->uta_available_atomic[i]);
            fpga->uta_available = i + 1;
            break;
        }
        else
            atomic_inc(&fpga->uta_available_atomic[i]);
    }
    
    return SUCCESS;
#else
    atomic_inc(&fpga->uta_available_atomic[0]);
    fpga->uta_available = 1;


    return SUCCESS;
#endif
}

int ibmuftian_open(struct inode *finode, struct file *filep)
{
    dev_info *device;
    ibmuftian_fd *fd_info;

    fd_info = kzalloc(sizeof(ibmuftian_fd), GFP_KERNEL);
    if(!fd_info)
    {
        PRINT(DBG_PRAM({ERR_DBG}), "failed to allocate memory to fd_info");
        return -ENOMEM;
    }

    device = container_of(finode->i_cdev, dev_info, dev_cdev);

    fd_info->dev = device;
    fd_info->filep = filep;
    if((fd_info->mln_no = dec_fd(device->fpga)) == FAILURE)
    {
        PRINT(DBG_PRAM({IBMUFTIAN_DBG}), "Driver busy max fd open");
        return -EBUSY;
    }
    
    filep->private_data = (void *) fd_info;

    return SUCCESS;
}

int ibmuftian_close(struct inode *finode, struct file *filep)
{
    ibmuftian_fd *fd_info = (ibmuftian_fd *)filep->private_data;
#ifdef MLN_ENVIRONMENT
    inc_fd(fd_info->dev->fpga,fd_info->mln_no);
#else
    inc_fd(fd_info->dev->fpga, 0);
#endif
    ZERO_AND_FREE(fd_info);

    return SUCCESS;
}

ssize_t ibmuftian_version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int length = 0;

    length += sprintf(buf, "%s: v%s\n", IBMUFTIAN_DESC, IBMUFTIAN_VERSION);

    return length; 
}

static int __init ibmuftian_init(void)
{
    int err = ZERO;

    PRINT(DBG_PRAM({IBMUFTIAN_DBG, ERR_DBG}), "loading the IBMUFTIAN driver");
    
    if(unlikely(err = initialize_driver()))
    {
        PRINT(DBG_PRAM({ERR_DBG}), "Driver initialization failed");
        return -1;
    }

    //initialize performance attr
    ibmuftian.performance.total_time.tv_sec = 0;
    ibmuftian.performance.total_time.tv_usec = 0;

    PRINT(DBG_PRAM({IBMUFTIAN_DBG}), "intializing sysfs done");
    
    err = ibmuftian_pci_register();
    if(unlikely(err != ZERO))
    {
        PRINT(DBG_PRAM({ERR_DBG}),"failed to regster pci devices");
        err = -ENOMEM;
    }

    PRINT(DBG_PRAM({IBMUFTIAN_DBG, LOAD_DBG}), "driver initialization done");
    
    return err;
}

static void __exit ibmuftian_cleanup(void)
{
    PRINT(DBG_PRAM({IBMUFTIAN_DBG}), "removing IBMUFTIAN driver");
    
    ibmuftian_pci_unregister();
    destroy_driver();

    PRINT(DBG_PRAM({IBMUFTIAN_DBG, LOAD_DBG}), "driver removed successfully");    
}

int register_driver(ibmuftian_fpga *fpga)
{
    int err;
    dev_info *device;
    char device_name[BOARD_NAME_LEN];
    dev_t prev_dev;
    
    PRINT(DBG_PRAM({IBMUFTIAN_DBG}), "registering driver");

    sprintf(device_name, IBMUFTIAN_NAME);
    device = &fpga->device_info;
        
    if(!fpga->id)
        err = alloc_chrdev_region(&device->dev, 0, 1, device_name);
    else
    {
        prev_dev = (get_fpga_by_id(fpga->id - 1))->device_info.dev;
	    device->dev=MKDEV(MAJOR(prev_dev),MINOR(prev_dev)+1);
	    err = register_chrdev_region(device->dev,1,device_name);
	}

    if(err)
    {
        PRINT(DBG_PRAM({ERR_DBG}), "failed to allocate char dev");
        return FAILURE;
    }
    
    cdev_init(&device->dev_cdev, &ibmuftian_fops);
	device->dev_cdev.owner = THIS_MODULE;

    err = cdev_add(&device->dev_cdev, device->dev, 1);
 
	if (unlikely(err))
	{
        PRINT(DBG_PRAM({ERR_DBG}), "failed to create and add cdev");
        goto error_exit;
	}
    
    PRINT(DBG_PRAM({IBMUFTIAN_DBG}), "driver registration done");

    return SUCCESS;

error_exit:
    unregister_chrdev_region(device->dev, 1);
    return err;
}

void unregister_driver(ibmuftian_fpga *fpga)
{
	dev_info *device = &fpga->device_info;
	 
 	cdev_del(&device->dev_cdev);
	unregister_chrdev_region(device->dev, 1);
}

static long ibmuftian_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    int err = ZERO;
    ibmuftian_fd *fd_info = (ibmuftian_fd *) filep->private_data;
    dev_info *device = fd_info->dev;
    struct pci_dev *pdev = device->pdev;
    ibmuftian_fpga *fpga = device->fpga;
    PRINT(DBG_PRAM({IOCTL_DBG}), "ioctl received");

    if (pci_channel_offline(pdev)) 
	{
        PRINT(DBG_PRAM({ERR_DBG}), "pci channel offline");
        return -EIO;
    }
		 
	 err = ioctl_call(fpga, cmd, arg);
     return err;
}

EXPORT_SYMBOL(init_function);
module_init(ibmuftian_init);
module_exit(ibmuftian_cleanup);

module_param_array(debug_areas, int, NULL, 0644);

MODULE_DESCRIPTION(IBMUFTIAN_DESC);
MODULE_AUTHOR("Kaustubh Gharpure <kmgharpu@us.ibm.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(IBMUFTIAN_VERSION);
