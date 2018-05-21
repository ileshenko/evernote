#include "ibmuftian_flash.h"
#include "ibmuftian.h"

static ssize_t store_bitfile(struct file *, struct kobject *, struct bin_attribute *, char *, loff_t, size_t);

static struct bin_attribute bitfile =
{
    .attr  = { .name = "bitfile", .mode = 0777 },
    .size  = TOTAL_CHIP_SIZE / 2, // Half the chip is the max bitfile size.
    .read  = NULL,
    .write = store_bitfile
};

static int reset_spi(ibmuftian_bar* bar)
{
    unsigned read_buff = 0x0;
    
	write_bar_addr(bar, SOFT_RESET_REG, SOFT_RESET_IP);
    write_bar_addr(bar, CONTROL_REG, ENABLE_SPI);
    write_bar_addr(bar, GLOBAL_INTERRUPT_REG, ENABLE_GLOBAL_INTERRUPT);
    
	read_buff = read_bar_addr(bar, INTERRUPT_STATUS_REG);
    if(read_buff & TOGGLE_CLEAR_DTR)
        write_bar_addr(bar, INTERRUPT_STATUS_REG, TOGGLE_CLEAR_DTR);

    return SUCCESS;
    PRINT(DBG_PRAM({FLASH_DBG}), "%8.8x readbuff is",read_buff);
}    

static int wait_write_comp(ibmuftian_bar* bar)
{
    int count = 0,status = SUCCESS;
    unsigned read_buff = 0x0;
    
	while(!(read_buff & 0x4))
    {
        read_buff = read_bar_addr(bar, INTERRUPT_STATUS_REG);
        udelay(10);
        count++;
        if(count == 20)
        {
            status = FAILURE;
            PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "write failed to complete");
            break;
        }
    }
    
    return status;
}

static int start_spi_operation(ibmuftian_bar* bar)
{
    write_bar_addr(bar, SLAVE_SEL_REG, SELECT_LINE_ENABLE);
    write_bar_addr(bar, CONTROL_REG, ENABLE_IP_SEND);
    
	return SUCCESS;
}

static unsigned read_reg_spi(ibmuftian_bar* bar, unsigned reg)
{
	unsigned ret_val = 0x0,read_buff = 0x0;
    int byte_num = 0,count = 0;
    
	read_buff = read_bar_addr(bar, INTERRUPT_STATUS_REG);
    if(read_buff & TOGGLE_CLEAR_DTR)
        write_bar_addr(bar, INTERRUPT_STATUS_REG, TOGGLE_CLEAR_DTR);
    write_bar_addr(bar, SLAVE_SEL_REG, DESELECT_SLAVE);
    write_bar_addr(bar, CONTROL_REG, SET_FIFO_RESET);
    write_bar_addr(bar, CONTROL_REG, ENABLE_SPI);
    write_bar_addr(bar, SPI_DTR, reg);
    write_bar_addr(bar, SPI_DTR, DUMMY_BYTE);
    start_spi_operation(bar);
    if(wait_write_comp(bar)) return -EINVAL;
    
	read_buff = 0xFFFFFFFF;
	
	while(!(read_buff & (0x4)))
    {
        read_buff = read_bar_addr(bar, SPI_STATUS_REG);
        PRINT(DBG_PRAM({FLASH_DBG}), "the status reg returned %8.8x",read_buff);
        mdelay(2);
        count++;
        if(count == 20)
        {
            PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "status reg never 0ed out");
            break; 
        }
    }

    byte_num = 0;
	read_buff = 0;
    
	read_buff = read_bar_addr(bar, SPI_STATUS_REG);
	
	while(!(read_buff & 0x5))
    {
		read_buff = read_bar_addr(bar, SPI_DRR);
        read_buff = read_bar_addr(bar, SPI_STATUS_REG);
	}
	
	ret_val = read_bar_addr(bar, SPI_DRR);
	ret_val = read_bar_addr(bar, SPI_DRR);
	
	write_bar_addr(bar, SLAVE_SEL_REG, DESELECT_SLAVE);
    
	return ret_val;
}

static int set_spi_command(ibmuftian_bar* bar, unsigned command)
{
    unsigned read_buff;
    
	read_buff = read_bar_addr(bar, INTERRUPT_STATUS_REG);
    if(read_buff & TOGGLE_CLEAR_DTR)
        write_bar_addr(bar, INTERRUPT_STATUS_REG, TOGGLE_CLEAR_DTR);
    write_bar_addr(bar, SLAVE_SEL_REG, DESELECT_SLAVE);
    write_bar_addr(bar, CONTROL_REG, SET_FIFO_RESET);
    write_bar_addr(bar, CONTROL_REG, ENABLE_SPI);
    write_bar_addr(bar, SPI_DTR, command);
    
	return SUCCESS;
}

static int send_spi_add(ibmuftian_bar* bar, unsigned add)
{
    int j;
    
	for (j = 3;j >= 0; j--)
    {
       write_bar_addr(bar, SPI_DTR, add >> (8 * j));
    }
    
	return SUCCESS;
}

static void send_spi_data_page(ibmuftian_bar* bar, void* buff,int size)
{
    int j;
    unsigned char byte;
    
	for (j = 0; j < size; j++)
    {
        byte = *(unsigned char *)(buff + j);
        write_bar_addr(bar, SPI_DTR, byte);
    }
    
}

static int read_spi_page(ibmuftian_flash *self, int half)
{
    unsigned read_buff = 0x0, size = SPI_PAGE_SIZE/2;
    int count = 0;
    unsigned char byte;
    while(size)
    {
		read_buff = read_bar_addr(self->bar4,SPI_DRR);
        byte = (unsigned char)read_buff;
        *(unsigned char *)(self->dma.verify_buf + (SPI_PAGE_SIZE/2 * half) + count) = byte;
		read_buff = read_bar_addr(self->bar4, SPI_STATUS_REG);
        size --;
		count ++;
	}
    
	return SUCCESS;
}

static int program_spi_page(ibmuftian_flash *self, unsigned addr, unsigned offset)
{
    unsigned read_buff = 0x0, count = 0;

	set_spi_command(self->bar4, PROGRAM_PAGE); 
    send_spi_add(self->bar4, addr);
    start_spi_operation(self->bar4);
    send_spi_data_page(self->bar4, self->dma.prog_buf + offset, SPI_PAGE_SIZE);
    if(wait_write_comp(self->bar4)) return -EINVAL;

	write_bar_addr(self->bar4, SLAVE_SEL_REG, DESELECT_SLAVE);
    
	while(!(read_buff & 0x80))
	{
	    read_buff = read_reg_spi(self->bar4, FLAG_STATUS_REG);
        count++;
	    if(count > 30)
		{
			PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "program spi page failed");
	        return FAILURE;
		}		
		mdelay(2);
	}
    	
	return SUCCESS;
}

static int verify_spi_page(ibmuftian_flash *self, unsigned addr, unsigned offset)
{
    unsigned i, j;
    unsigned char dummy_bytes[SPI_PAGE_SIZE] = {DUMMY_BYTE};
    
    set_spi_command(self->bar4, FAST_READ_PAGE);
    send_spi_add(self->bar4, addr);
    start_spi_operation(self->bar4);
    
	for(j = 0; j < 2;j++)
	{
        send_spi_data_page(self->bar4, (void*)dummy_bytes, SPI_PAGE_SIZE/2);
        if(wait_write_comp(self->bar4)) return -EINVAL;
        if(!j)
		{
		    for(i = 0; i < 5; i++)
              read_bar_addr(self->bar4,SPI_DRR);
                
		}
        read_spi_page(self, j);
	}
	
	write_bar_addr(self->bar4, SLAVE_SEL_REG, DESELECT_SLAVE);
    
    if(memcmp(self->dma.prog_buf + offset,self->dma.verify_buf,SPI_PAGE_SIZE))
    {
        for(i = 0; i < 256; i++)
        {
            PRINT(DBG_PRAM({FLASH_DBG}), "the actual data is %x",*(unsigned char *)(self->dma.prog_buf + offset + i));
            PRINT(DBG_PRAM({FLASH_DBG}), "the received data is %x",*(unsigned char *)(self->dma.verify_buf + i));
        }
        return FAILURE;
    }
    else
        return SUCCESS;
        
}

static int erase_spi_sector(ibmuftian_bar *bar, unsigned addr)
{
    unsigned read_buff = 0,count= 0;
    
	set_spi_command(bar, ERASE_SECTOR);
    send_spi_add(bar, addr);
    start_spi_operation(bar);
    if(wait_write_comp(bar)) return -EINVAL;
    
	write_bar_addr(bar, SLAVE_SEL_REG, DESELECT_SLAVE);
	
	while((read_buff != 0x81))
	{
	    read_buff = read_reg_spi(bar, FLAG_STATUS_REG);
	    count++;
	    if(count > 20)
		{
			PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "erase failed");
	        return FAILURE;
		}
	    mdelay(10);
	}
    	
    return SUCCESS;
}

static int execute_spi_command(ibmuftian_bar* bar, unsigned command)
{
    set_spi_command(bar,command); 
    start_spi_operation(bar);
    if(wait_write_comp(bar)) return -EINVAL;
    
	write_bar_addr(bar, SLAVE_SEL_REG, DESELECT_SLAVE);
    
	return SUCCESS;
}

static int alloc_dma_buff(ibmuftian_flash *flash)
{
    struct pci_dev* pdev = flash->bar4->pdev;

    flash->dma.prog_buf = flash->dma.prog_buf ? flash->dma.prog_buf : dma_alloc_coherent(&pdev->dev, SECTOR_SIZE, &flash->dma.prog_dma_addr, GFP_KERNEL | GFP_DMA32);
    if(unlikely(!flash->dma.prog_buf))
    {
        PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "failed to allocate memory for flash program buffer");
        flash->dma.prog_buf = NULL;
        return FAILURE;
    }
    flash->dma.verify_buf = flash->dma.verify_buf ? flash->dma.verify_buf : dma_alloc_coherent(&pdev->dev, SECTOR_SIZE, &flash->dma.verify_dma_addr, GFP_KERNEL | GFP_DMA32);
    if(unlikely(!flash->dma.verify_buf))
    {
        PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "failed to allocate memory for flash verify buffer");
        flash->dma.verify_buf = NULL;
        goto error_exit;
    }

    return SUCCESS;

error_exit:
    dma_free_coherent(&pdev->dev, SECTOR_SIZE, flash->dma.prog_buf, flash->dma.prog_dma_addr);
    flash->dma.prog_buf = NULL;

    return FAILURE;
}

void begin_flash(ibmuftian_flash *flash)
{
    spin_lock(&(flash->flashing));
    flash->flash_in_progress = 1;
    spin_unlock(&(flash->flashing));
}

void flash_complete(ibmuftian_flash *flash)
{
    spin_lock(&(flash->flashing));
    flash->flash_in_progress = 0;
    spin_unlock(&(flash->flashing));
}

int is_flash_in_progress(ibmuftian_flash *flash)
{
    int ret;

    spin_lock(&(flash->flashing));
    ret = flash->flash_in_progress;
    spin_unlock(&(flash->flashing));

    return ret; 
}

static void cleanup(ibmuftian_flash *flash)
{
    flash->dma.prog_offset = 0;
    flash->dma.sector_offset = 0;

    flash_complete(flash);

    sema_init(&flash->prog_sync, 0);
}

static ssize_t store_bitfile(struct file *file, struct kobject *kobj, struct bin_attribute *bin_attr, char *buf, loff_t ofs, size_t count)
{
    ibmuftian_flash *flash = to_flash(kobj);
    int res = SUCCESS, page_offset = 0 , failed_sector = 0;
    unsigned addr, bytes_flashed, page_fail;
    PRINT(DBG_PRAM({FLASH_DBG}), "count is %zd offset is %lld", count, ofs);

    if(unlikely(!flash || !buf))
    {
        res = -EFAULT;
        goto error_exit;
    }

    if(down_interruptible(&flash->prog_sync))
    {
        PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "failed to program flash: flash in use");
        flash->prog_state = FAILED;
        flash->fail_code = FLASH_LOCKED;
        res = -EINTR;
        goto error_exit;
    }
    up(&flash->prog_sync);

    if(flash->dma.prog_offset >= flash->prog_size)
    {
        PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "failed to program flash: image too big");
        flash->prog_state = FAILED;
        flash->fail_code = FILE2BIG;
        res = -EFBIG;
        goto error_exit;
    }
    
    bytes_flashed = flash->dma.sector_offset + count > SECTOR_SIZE ? SECTOR_SIZE - flash->dma.sector_offset : count;
    memcpy(flash->dma.prog_buf, buf, bytes_flashed);
    flash->dma.sector_offset += bytes_flashed;

    PRINT(DBG_PRAM({FLASH_DBG}), "bytes flashed = %d sector offset %zd", bytes_flashed, flash->dma.sector_offset);

    if(flash->dma.sector_offset < SECTOR_SIZE && flash->dma.prog_offset + flash->dma.sector_offset < flash->prog_size)
    {
        PRINT(DBG_PRAM({FLASH_DBG}), "buffered %d bytes", bytes_flashed);
        return bytes_flashed;
    }

    if(flash->image_code == WORKING_IMAGE)
        addr = PROG_BASE_ADDR_MAIN;
    else if(flash->image_code == GOLDEN_IMAGE)
        addr = PROG_BASE_ADDR_GOLDEN;
    else
        return -EFAULT;
    
    repro_sector:
        if(failed_sector > 2)
        {
            PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "failed to program flash: retry exceeded");
            goto error_exit;
        }
        flash->prog_state = ERASING;    
        if(execute_spi_command(flash->bar4, FLASH_WRITE_ENABLE)) return -EFAULT;
        if(erase_spi_sector(flash->bar4,addr + flash->dma.prog_offset))
        {
            flash->prog_state = FAILED;
            flash->fail_code = FAIL_ER_PG;
            res = -EFAULT;
            PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "erase sector %lu failed\n",flash->dma.prog_offset/SECTOR_SIZE);
            goto error_exit;
        }
        
        page_offset = 0;
        page_fail = 0;
        while(page_offset < 256)
        {
            unsigned offset = (page_offset * SPI_PAGE_SIZE);
            flash->prog_state = PROGRAMING;
            if(execute_spi_command(flash->bar4, FLASH_WRITE_ENABLE)) return -EFAULT; 
            if(program_spi_page(flash, offset + addr + flash->dma.prog_offset, offset)) 
            {
                flash->prog_state = FAILED;
                flash->fail_code = FAIL_PRO_PG;
                res = -EFAULT;
                PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "program page failed %lu ", 1 + (flash->dma.prog_offset/SPI_PAGE_SIZE));
            }
            else
            {
                flash->fail_code = NO_CODE;
            }
            
            flash->prog_state = VERIFYING; 
            if(verify_spi_page(flash,offset + addr + flash->dma.prog_offset, offset)) 
            {
                flash->prog_state = FAILED;
                flash->fail_code = FAIL_VR_PG;
                res = -EFAULT;
                PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "verify page %lu... failed at offset %zd \n", 1 + (flash->dma.prog_offset/SPI_PAGE_SIZE), flash->dma.prog_offset + offset + addr);
                page_fail++;
            }
            else
            {
                if(page_fail)
                    page_fail = 0;
                if(failed_sector)
                    failed_sector = 0;
            }
            
            if (!page_fail)
                page_offset ++;
            else if(page_fail > 1)
            {
                failed_sector ++;
                PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "failed to program a sector going for retry");
                goto repro_sector;
            }
        }

    flash->flash_pct = (100 * (flash->dma.prog_offset+ SECTOR_SIZE)) / flash->prog_size;
    flash->dma.prog_offset += flash->dma.sector_offset < SECTOR_SIZE ? flash->dma.sector_offset : SECTOR_SIZE;
    flash->dma.sector_offset = 0;
    
    if(flash->dma.prog_offset < flash->prog_size)
    {
        memset(flash->dma.prog_buf, 0xFF, SECTOR_SIZE);
    }
    else
    {
        flash->prog_state = SUCCEEDED;
        flash->fail_code = NO_CODE;
        PRINT(DBG_PRAM({FLASH_DBG}), "Flashed programmed successfully"); 
        cleanup(flash);
    }
    
    PRINT(DBG_PRAM({FLASH_DBG}), "gng back for data %d bytes remaining %zd",bytes_flashed,(flash->prog_size - flash->dma.prog_offset));
    return bytes_flashed;

error_exit:
    PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "failed to program flash exiting");
    cleanup(flash);
    return res;
}

static ssize_t attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    flash_attr *flash_attribute = to_flash_attr(attr);
    ibmuftian_flash *flash = to_flash(kobj);

    return flash_attribute->show ? flash_attribute->show(flash, flash_attribute, buf) : -EIO;
}

static ssize_t attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    flash_attr *flash_attribute = to_flash_attr(attr);
    ibmuftian_flash *flash = to_flash(kobj);

    return flash_attribute->store ? flash_attribute->store(flash, flash_attribute, buf, count) : -EIO;
}

static void release(struct kobject *kobj)
{
    ibmuftian_flash *flash;
    struct pci_dev *pdev;
    

    if (likely(kobj))
    {
        flash = to_flash(kobj);
        pdev = flash->bar4->pdev;
        dma_free_coherent(&pdev->dev, SECTOR_SIZE, flash->dma.prog_buf, flash->dma.prog_dma_addr);
        dma_free_coherent(&pdev->dev, SECTOR_SIZE, flash->dma.verify_buf, flash->dma.verify_dma_addr);
        flash->dma.prog_buf = NULL;
        flash->dma.verify_buf = NULL;
        ZERO_AND_FREE(flash);
    }
}

static int init_flash(ibmuftian_flash *flash)
{
    if(is_flash_in_progress(flash))
    {
        PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "flash initialization failed: flash busy");
        return -EBUSY;
    }

    if(unlikely(flash->image_code != WORKING_IMAGE && flash->image_code != GOLDEN_IMAGE))
    {
        PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "flash initialization failed: image code does not match");
        return -EINVAL;
    }

    if (unlikely(!flash->prog_size))
    {
        PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "flash initialization failed: program size 0");
        return -EINVAL;
    }
    
    alloc_dma_buff(flash);

    if (reset_spi(flash->bar4)) return -EFAULT;
    if(execute_spi_command(flash->bar4, FOUR_BYTE_ADDR)) return -EFAULT;
    if(execute_spi_command(flash->bar4, FOUR_BYTE_ADDR)) return -EFAULT;

    flash->prog_state = NO_OPERATION;
    flash->fail_code = NO_CODE;
    flash->flash_pct = 0;
    flash->dma.prog_offset = 0;
    flash->dma.sector_offset = 0;
    
    begin_flash(flash);

    up(&flash->prog_sync);

    return SUCCESS;    
}

static ssize_t show_status(ibmuftian_flash *flash, char *buf)
{
    char const * const prog_states_str[] = { PROG_STATE_LIST };
    char const * const fail_codes_str[] = { FAIL_CODE_LIST };

    if(unlikely(flash->prog_state < UNKNOWN))
    {
        if(flash->prog_state == SUCCEEDED)
        {
            return safe_sprintf(buf, SPI_PAGE_SIZE, "%s\n", prog_states_str[flash->prog_state]);
        }
        else
        {
            return safe_sprintf(buf, SPI_PAGE_SIZE, "STATE - %s \nFAIL CODE - %s \n%u%% done\n", prog_states_str[flash->prog_state], fail_codes_str[flash->fail_code], flash->flash_pct);
        }
    }
    else
    {
        return safe_sprintf(buf, SPI_PAGE_SIZE, "Unknown state\n");
    }
}

static ssize_t flash_store(ibmuftian_flash *flash, flash_attr *attr, const char *buf, size_t count)
{
    u32 buffer_val, res;

    if (unlikely(!flash) || unlikely(!attr) || unlikely(!buf) || unlikely(!count)) return -EINVAL;

    if (!strcmp(attr->attr.name, "prog_size"))
    {
        buffer_val = __cstr2ulong(buf);
        if (!is_flash_in_progress(flash))
            flash->prog_size = buffer_val;
        else
            count = -EINVAL;
    }
    else if (!strcmp(attr->attr.name, "image_type"))
    {
        buffer_val = __cstr2ulong(buf);
        if (!is_flash_in_progress(flash))
        {
            if (unlikely(buffer_val != WORKING_IMAGE && buffer_val != GOLDEN_IMAGE))
            {
                PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "Bad program code: %d", buffer_val);
                count = -EINVAL;
            }
            else
                flash->image_code = buffer_val;
        }
        else
            count = -EINVAL;
    }
    else if (!strcmp(attr->attr.name, "init_flash"))
    {
        buffer_val = __cstr2ulong(buf);
        if (buffer_val)
        {
            res = init_flash(flash);
            count = res ? res : count;
        }
    }

    return (ssize_t) count;
}


static ssize_t flash_show(ibmuftian_flash *flash, flash_attr *attr, char *buf)
{
    int num_chars;

    if (unlikely(!flash) || unlikely(!buf) || unlikely(!attr)) return 0;

    
    if (!strcmp(attr->attr.name, "prog_size"))
    {
        num_chars = safe_sprintf(buf, SPI_PAGE_SIZE, "%u\n", flash->prog_size);
    }
    else if (!strcmp(attr->attr.name, "status"))
    {
        num_chars = show_status(flash, buf);
    }
    else
    {
        num_chars = safe_sprintf(buf, SPI_PAGE_SIZE, "%s() - method for '%s' not yet implemented\n", __func__, attr->attr.name);
    }

    return num_chars;
}

int create_flash(ibmuftian_fpga *fpga)
{
    ibmuftian_flash *new_flash = NULL;

    static flash_attr image_type = __ATTR(image_type, S_IWUGO, NULL, flash_store);
    static flash_attr prog_size = __ATTR(prog_size, S_IRUGO | S_IWUGO, flash_show, flash_store);
    static flash_attr init_flash = __ATTR(init_flash, S_IWUGO, NULL, flash_store);
    static flash_attr status = __ATTR(status, S_IRUGO, flash_show, NULL);
    
    static struct attribute *flash_attrs[] =
    {
        &image_type.attr,
        &prog_size.attr,
        &init_flash.attr,
        &status.attr,
        NULL,
    };
    
    static struct sysfs_ops flash_sysfs_ops = { .show  = attr_show, .store = attr_store };
    
    static struct kobj_type flash_ktype =
    {
        .release        = release,
        .sysfs_ops      = &flash_sysfs_ops,
        .default_attrs  = flash_attrs,
    };

    new_flash = kzalloc(sizeof(ibmuftian_flash), GFP_KERNEL);

    fpga->flash = new_flash;

    new_flash->id = fpga->id;
    new_flash->bar4 = fpga->bar.bar4;
    new_flash->prog_state = NO_OPERATION;
    new_flash->fail_code = NO_CODE;
    sema_init(&new_flash->prog_sync, 0);

    if (unlikely(kobject_init_and_add(&new_flash->kobj, &flash_ktype, &fpga->kobj, "flash")))
    {
        PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "kobject init/add failed");
        goto error_exit;
    }

    if (sysfs_create_bin_file(&new_flash->kobj, &bitfile))
    {
        PRINT(DBG_PRAM({FLASH_DBG, ERR_DBG}), "sysfs create failed");
        goto error_exit;
    }
    
    return SUCCESS;

error_exit:
    kobject_put(&new_flash->kobj);
    fpga->flash = NULL;
    return FAILURE;    
}

void flash_delete(ibmuftian_flash *flash)
{
    if(likely(flash))
    {
        PRINT(DBG_PRAM({FLASH_DBG}), "removing flash"); 
        sysfs_remove_bin_file(&flash->kobj, &bitfile);
        kobject_put(&flash->kobj);
    }
}
