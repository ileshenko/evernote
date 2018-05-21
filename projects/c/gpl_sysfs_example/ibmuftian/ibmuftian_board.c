#include <linux/kobject.h>

#include "ibmuftian_board.h"
#include "ibmuftian.h"
#include "ibmuftian_containers.h"


static ssize_t show_vpd(struct file *, struct kobject *, struct bin_attribute *, char *, loff_t, size_t);

static struct bin_attribute vpd_attr =
{
    .attr  = { .name = "vpd", .mode = 0444  },
    .size  = VPD_SIZE,
    .read  = show_vpd
};

static ssize_t attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    board_attr *board_attribute = to_board_attr(attr);
    ibmuftian_board *board = to_board(kobj);

    return board_attribute->show ? board_attribute->show(board, board_attribute, buf) : -EIO;
}

static ssize_t attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    board_attr *board_attribute = to_board_attr(attr);
    ibmuftian_board *board = to_board(kobj);

    return board_attribute->store ? board_attribute->store(board, board_attribute, buf, count) : -EIO;
}

static ssize_t show_board(ibmuftian_board *board, struct board_attr *attr, char *buf)
{
    int num_chars;

    if (unlikely(!board) || unlikely(!buf) || unlikely(!attr)) return 0;

    if (!strcmp(attr->attr.name, "serno"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%s\n", board->vpd_info.serno[0] ? board->vpd_info.serno : "NO SERNO");
    }
    else if (!strcmp(attr->attr.name, "hwrev"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%s\n", board->vpd_info.hwrev[0] ? board->vpd_info.hwrev : "NO HWREV");
    }
    else if (!strcmp(attr->attr.name, "desc"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%s\n", board->vpd_info.desc[0] ? board->vpd_info.desc : "NO DESC");
    }
	else if (!strcmp(attr->attr.name, "board_name"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%s\n", board->board_name);
    }
    else
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "method for '%s' not yet implemented\n", attr->attr.name);
    }

    return num_chars;
}

static void init_vpd_fields(ibmuftian_board *board)
{
    unsigned char hwrev;

    memcpy(&board->vpd_info.serno[0], board->vpd_info.vpd_buf + CARD_PREFIX_SERIAL_NUMBER_OFS, CARD_PREFIX_SERIAL_NUMBER_LEN);
    memcpy(&board->vpd_info.serno[CARD_PREFIX_SERIAL_NUMBER_LEN], board->vpd_info.vpd_buf + CARD_SERIAL_NUMBER_OFS, CARD_SERIAL_NUMBER_LEN);
    board->vpd_info.serno[CARD_PREFIX_SERIAL_NUMBER_LEN + CARD_SERIAL_NUMBER_LEN] = '\0';

    hwrev = *(unsigned char *) (board->vpd_info.vpd_buf + HARDWARE_REVISION_LEVEL_OFS);
    safe_sprintf(board->vpd_info.hwrev, sizeof(board->vpd_info.hwrev), "0x%02X", (unsigned) hwrev);

    memcpy(board->vpd_info.desc, board->vpd_info.vpd_buf +IBM_PRODUCT_NAME_OFS, IBM_PRODUCT_NAME_LEN);
    board->vpd_info.desc[IBM_PRODUCT_NAME_LEN] = '\0';

    PRINT(DBG_PRAM({BOARD_DBG}), "vpd fields initialized"); 
}

static ssize_t show_vpd(struct file *file, struct kobject *kobj, struct bin_attribute *bin_attr, char *buf, loff_t ofs, size_t count)
{
    static unsigned bytes_total = 0;
    unsigned bytes_consumed = 0;
    ibmuftian_board * board = to_board(kobj);

    if (unlikely(!board) || unlikely(!buf))
    {
        PRINT(DBG_PRAM({BOARD_DBG, ERR_DBG}), "board is %p, buf is %p", board, buf);
        return -EINVAL;
    }

    bytes_consumed = memory_read_from_buffer(buf, count, &ofs, board->vpd_info.vpd_buf, VPD_SIZE);
    bytes_total   += bytes_consumed;

    return bytes_consumed;
}

struct kset* get_board_container(void)
{
    return likely(ibmuftian.board_c) ? ibmuftian.board_c->kobjset : NULL;
}

int get_board_count(void)
{
    return likely(ibmuftian.board_c)? atomic_read(&(ibmuftian.board_c->count)): 0;
}

ssize_t board_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_board_count());
}

void add_board(void)
{
    atomic_add(1, &(ibmuftian.board_c->count));
}

void remove_board(void)
{
    atomic_dec(&(ibmuftian.board_c->count));
}

void remove_board_l(int id)
{
    ibmuftian_board *board_ptr = NULL, *n_ptr = NULL;
    
    list_for_each_entry_safe(board_ptr, n_ptr, &(ibmuftian.board_c->board_l), list)
    {
        if(board_ptr->id == id)
        {
            list_del(&board_ptr->list);
            break;
        }
    }
    
}

static void release(struct kobject *kobj)
{
    ibmuftian_board *board = to_board(kobj);
    if (likely(board))
    {
        remove_board();
        ZERO_AND_FREE(board);
    }
}

static int alloc_read_vpd(ibmuftian_board *board)
{
    const u8 chan_code = I2C_MUX_CHAN_2; // bit[3] = enable, bits[2:0] are the channel
	i2c_bd mux_bd =
	{
		.i2c			  = board->i2c_cntrl,
		.i2c_addr		  = I2C_MUX_CNT,
		.page_size		  = ONE_PAGE,
		.dev_offset 	  = ZERO_DEV_OFFSET,
		.offset_size	  = ZERO_OFFSET_SIZE
	};
	i2c_bd vpd_bd =
	{
		.i2c			  = board->i2c_cntrl,
		.i2c_addr		  = I2C_VPD,
		.page_size		  = VPD_BD_PG_SIZE,
		.dev_offset 	  = ZERO_DEV_OFFSET,
		.offset_size	  = 2
	};

	board->vpd_info.vpd_buf = kmalloc(VPD_SIZE, GFP_KERNEL);
    if (unlikely(!board->vpd_info.vpd_buf))
	{
    	PRINT(DBG_PRAM({BOARD_DBG, ERR_DBG}), "failed to allocate memory to vpd buffer");
        return FAILURE;
    }
	
	if(lock_i2c_controller(board->i2c_cntrl))
	{
	    PRINT(DBG_PRAM({BOARD_DBG, ERR_DBG}), "failed to acquire i2c controller");
	    return FAILURE;
	}

	if (i2c_write(&mux_bd, &chan_code, sizeof chan_code, NULL))
	{
		PRINT(DBG_PRAM({BOARD_DBG, ERR_DBG}), "failed to set mux to channel %d", chan_code);
        return FAILURE;
	}
	else if (i2c_read(&vpd_bd, board->vpd_info.vpd_buf, VPD_SIZE, NULL))
	{
		PRINT(DBG_PRAM({BOARD_DBG, ERR_DBG}), "Failed to read VPD into buffer");
		return FAILURE;
	}
	release_i2c_controller(board->i2c_cntrl);

    init_vpd_fields(board);
	
	return SUCCESS;
	
}

int create_board(struct pci_dev * pdev)
{
    int res, hmc = 0, trial = 0;
    ibmuftian_board *new_board = NULL;

    int board_count = atomic_read(&(ibmuftian.board_c->count));

    static board_attr serno = __ATTR(serno, S_IRUGO, show_board, NULL);
    static board_attr hwrev = __ATTR(hwrev, S_IRUGO, show_board, NULL);
    static board_attr desc = __ATTR(desc , S_IRUGO, show_board, NULL);
    static board_attr board_name = __ATTR(board_name , S_IRUGO, show_board, NULL);

    static struct attribute *board_attrs[] =
    {
        &serno.attr,
        &hwrev.attr,
        &desc.attr,        
        &board_name.attr,
        NULL
    };

    static struct sysfs_ops board_sysfs_ops = { .show  = attr_show, .store = attr_store };

    static struct kobj_type board_ktype =
    {
        .release = release,
        .sysfs_ops = &board_sysfs_ops,
        .default_attrs = board_attrs,
    };

    PRINT(DBG_PRAM({BOARD_DBG}), "creating board");

    PRINT(DBG_PRAM({BOARD_DBG}), "allocating space for board");

    new_board = kzalloc(sizeof(ibmuftian_board), GFP_KERNEL);
    list_add(&new_board->list, &(ibmuftian.board_c->board_l));
       
    if (unlikely(!new_board))
    {
        
        PRINT(DBG_PRAM({BOARD_DBG, ERR_DBG}), "failed to allocate memory for board");
        return FAILURE;
    }
    
    new_board->kobj.kset = get_board_container();
    new_board->id = board_count;

    add_board();

    PRINT(DBG_PRAM({BOARD_DBG}), "board added initalizing sysfs");
    
    if (unlikely(kobject_init_and_add(&new_board->kobj, &board_ktype, NULL, "%d", new_board->id)))
    {
        PRINT(DBG_PRAM({BOARD_DBG, ERR_DBG}), "kobject init/add failed");
        goto exit_and_put;
    }

    if (sysfs_create_bin_file(&new_board->kobj, &vpd_attr)) goto exit_and_put;

    PRINT(DBG_PRAM({BOARD_DBG}), "sysfs created");

    if(create_fpga(new_board, pdev)) goto exit_and_put;

    PRINT(DBG_PRAM({BOARD_DBG}), "fpga,engns created");

    safe_sprintf(new_board->board_name, BOARD_NAME_LEN, "%s", new_board->board_type == FIN ? BOARD_NAME_FIN : BOARD_NAME_WRASSE);

    if(new_board->board_type == FIN)
    {
        if(create_i2c(new_board)) goto exit_and_put;

        PRINT(DBG_PRAM({BOARD_DBG}), "sysfs i2c created");

        write_bar_addr(new_board->bar4,FPGA_I2C_ACCESS);
        PRINT(DBG_PRAM({BOARD_DBG}), "fpga granted i2c access");

        if (alloc_read_vpd(new_board))
        {
            PRINT(DBG_PRAM({BOARD_DBG, ERR_DBG}), "failed to read VPD contents");
            goto exit_and_put;
        }

        PRINT(DBG_PRAM({BOARD_DBG}), "starting HMC training");

        while(hmc < HMC_NUM)
        {
            res = trainHMC(new_board->fpga->bar.bar4, new_board->i2c_cntrl, hmc);
            if(!res)
            {
               hmc++;
               trial = ZERO;
               PRINT(DBG_PRAM({BOARD_DBG}), "HMC %d TRAINED",hmc);
            }
            else
                trial++;
            if(trial > MAX_TRIALS)
            {
                PRINT(DBG_PRAM({BOARD_DBG}), "HMC %d not trained after multiple attempts",hmc);
                trial = ZERO;
                hmc++;
            }
        }
    }
    pci_set_drvdata(pdev, new_board);

    return SUCCESS;

exit_and_put:
    PRINT(DBG_PRAM({BOARD_DBG, ERR_DBG}), "board error area");
    if(new_board->board_type == FIN)
    {
        write_bar_addr(new_board->bar4,BOARD_I2C_ACCESS);
        i2c_delete(new_board->i2c_cntrl);
    }
    kobject_put(&new_board->kobj);
    remove_board();
    list_del(&new_board->list);
    ZERO_AND_FREE(new_board);
    return FAILURE;
}	

void board_delete(ibmuftian_board * board)
{
    PRINT(DBG_PRAM({BOARD_DBG}),"Removing board %d", board->id);
    if(board->board_type == FIN)
    {
        write_bar_addr(board->bar4,BOARD_I2C_ACCESS);
        i2c_delete(board->i2c_cntrl);
    }
    fpga_delete(board->fpga);
    remove_board_l(board->id);
    kobject_put(&board->kobj);
}
