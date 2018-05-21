#include "ibmuftian_i2c.h"
#include "ibmuftian.h"

static int wait_for_acknak(ibmuftian_i2c *i2c, int wanted_ack)
{
    unsigned const one_us_attempts = 10000;
    unsigned attempt;
    i2c_stats stats;

    for (attempt = 0; attempt < one_us_attempts; ++attempt)
    {
        // RxACK will be '0' for ACK received, '1' for NAK received.
        stats.all = read_bar_addr(i2c->bar4, I2C_STAT);
        if (!stats.bit.tip && stats.bit.iflag) return wanted_ack ? !stats.bit.rxack : stats.bit.rxack;
        udelay(1);
    }

    PRINT(DBG_PRAM({I2C_DBG}), "Timed-out waiting or %s, status = 0x02%X", wanted_ack ? "ack" : "nak", (unsigned) stats.all);

    // A timeout is treated as a NAK. Subsequent I2C ops. will fail on TIP=1.
    return wanted_ack ? 0 : 1;
}

static void generate_nak_stop(ibmuftian_i2c *i2c)
{
    write_bar_addr(i2c->bar4, I2C_CMD, I2C_CMD_RD|I2C_CMD_ACK|I2C_CMD_STO);
}

static int controller_busy(ibmuftian_i2c *i2c, unsigned *stat_val)
{
    i2c_stats stats = { .all = read_bar_addr(i2c->bar4, I2C_STAT) };
    if (stat_val) *stat_val = (unsigned) stats.all;

    return stats.bit.tip || stats.bit.busy;
}

static int write_dev_addr(ibmuftian_i2c *i2c, u32 i2c_addr, u32 dev_addr, u32 dev_addr_len, int for_read)
{
    struct { u8 octet; u8 ctrl; } tx[6];
    const u8 rd = 0x01;
    const u8 wr = 0x00;
    unsigned num_bytes = 0;
    unsigned idx;
    unsigned val;
    u32 i2c_addr_dir;

    if (controller_busy(i2c, &val))
    {
        PRINT(DBG_PRAM({I2C_DBG}), "I2C controller claims the bus is busy STAT[%02X], try again later", val);
        return FAILURE;
    }

    // Need the bus address shifted up into top seven bits to make room for
    // direction bit which will always be zero for writes and one for reads
    // that are continuations (dev_addr_len == 0);
    i2c_addr_dir = i2c_addr << 1;
    i2c_addr_dir = dev_addr_len ? i2c_addr_dir : i2c_addr_dir | (for_read ? rd : wr);

    // Byte 0: I2C bus address + write bit.
    tx[num_bytes].octet  = (u8) i2c_addr_dir;
    tx[num_bytes++].ctrl = I2C_CMD_STA | I2C_CMD_WR;

    // Optional Bytes: 1 & 2: One or two byte device address/offset.
    if (dev_addr_len == 1)
    {
        tx[num_bytes].octet  = (u8) dev_addr;
        tx[num_bytes++].ctrl = I2C_CMD_WR;
    }
    else if (dev_addr_len == 2)
    {
        tx[num_bytes].octet  = (u8) (dev_addr >> 8);
        tx[num_bytes++].ctrl = I2C_CMD_WR;
        tx[num_bytes].octet  = (u8)  dev_addr;
        tx[num_bytes++].ctrl = I2C_CMD_WR;
    }
    else if (dev_addr_len == 4)
    {
        tx[num_bytes].octet  = (u8) (dev_addr >> 24);
        tx[num_bytes++].ctrl = I2C_CMD_WR;
        tx[num_bytes].octet  = (u8) (dev_addr >> 16);
        tx[num_bytes++].ctrl = I2C_CMD_WR;
        tx[num_bytes].octet  = (u8) (dev_addr >> 8);
        tx[num_bytes++].ctrl = I2C_CMD_WR;
        tx[num_bytes].octet  = (u8)  dev_addr;
        tx[num_bytes++].ctrl = I2C_CMD_WR;
    }

    // Optional Byte 3: Need to re-address the device with read bit set. Only necessary
    // on a random access read where we've just set the address/offset above.
    if (dev_addr_len && for_read)
    {
        tx[num_bytes].octet  = (u8) (i2c_addr_dir | rd);
        tx[num_bytes++].ctrl = I2C_CMD_STA | I2C_CMD_WR; // Repeated start.
    }
    
    write_bar_addr(i2c->bar4, I2C_CTRL, 0x00);
    write_bar_addr(i2c->bar4, I2C_CTRL ,0x80);
     
    // Send it!
    for (idx = 0; idx < num_bytes; ++idx)
    {
        write_bar_addr(i2c->bar4, I2C_TX , tx[idx].octet);
        write_bar_addr(i2c->bar4, I2C_CMD, tx[idx].ctrl);
        if (wait_for_ack(i2c)) continue;

        PRINT(DBG_PRAM({I2C_DBG}), "Byte %u/%u to device at 0x%02X was unexpectedly NAK'd", idx + 1, num_bytes, (unsigned) i2c_addr);
        generate_nak_stop(i2c);
        return FAILURE;
    }

    return SUCCESS;
}

static int read_dev_data(ibmuftian_i2c *i2c, u32 i2c_addr, u32 dev_addr, u32 dev_addr_len, void *buf, unsigned size)
{
    int const last  = size - 1;
    u8       *cbuf  = buf;
    unsigned  idx;
    if (write_dev_addr(i2c, i2c_addr, dev_addr, dev_addr_len, I2C_READ)) return FAILURE;

    // Data payload, issue STO on last data byte. We are driving ACK/NAK.
    for (idx = 0; idx < size; ++idx)
    {
        if (idx == last)
        {
            // RD w/ ACK means read and nak since nak=1.
            write_bar_addr(i2c->bar4, I2C_CMD, I2C_CMD_RD|I2C_CMD_ACK|I2C_CMD_STO);
            if (!wait_for_nak(i2c))
            {
                //prn_i2c("Should have read NAK while reading byte %u/%u from device at 0x%02X was NAK'd", idx + 1, size, (unsigned) i2c_addr);
                generate_nak_stop(i2c);
                return FAILURE;
            }
        }
        else
        {
            // RD w/o ACK means read and ack since ack=0.
            write_bar_addr(i2c->bar4, I2C_CMD, I2C_CMD_RD);
            if (!wait_for_ack(i2c))
            {
                PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "Should have read ACK while reading byte %u/%u from device at 0x%02X was NAK'd", idx + 1, size, (unsigned) i2c_addr);
                generate_nak_stop(i2c);
                return FAILURE;
            }
        }

        cbuf[idx] = read_bar_addr(i2c->bar4, I2C_RX);
    }
    
    *(u32*)buf = __builtin_bswap32(*((u32*)buf));
    return SUCCESS;
}

static int write_dev_data(ibmuftian_i2c *i2c, u32 i2c_addr, u32 dev_addr, u32 dev_addr_len, void *buf, unsigned size)
{
    int const last  = size - 1;
    u8 *cbuf  = buf;
    unsigned  idx;
    if (write_dev_addr(i2c, i2c_addr, dev_addr, dev_addr_len, I2C_WRITE)) return FAILURE;

    // Data payload, issue STO on last data byte.
    for (idx = 0; idx < size; ++idx)
    {
        write_bar_addr(i2c->bar4, I2C_TX , cbuf[idx]);
        write_bar_addr(i2c->bar4, I2C_CMD, idx == last ? I2C_CMD_WR|I2C_CMD_STO : I2C_CMD_WR);
        if (wait_for_ack(i2c)) continue;

        PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "Byte %u/%u to device at 0x%02X was unexpectedly NAK'd", idx + 1, size, (unsigned) i2c_addr);
        generate_nak_stop(i2c);
        return FAILURE;
    }

    return SUCCESS;
}

static int i2c_xfer(i2c_bd *bd, void *buf, unsigned size, unsigned *count, int read)
{
    unsigned total_bytes;
    unsigned start;
    
    if(size > 1 && !read)
    *(u32*)buf = __builtin_bswap32(*((u32*)buf));

    if (unlikely(!bd) || unlikely(!buf) || unlikely(!bd->i2c->ready) || unlikely(!size))
    {
        PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "I2C buffer descriptor null bd %p buf %p size %d", bd, buf, size);
    }

    // Pre-reqs. satisfied. Write the address into the device and then read the data.
    total_bytes = 0;
    start       = bd->dev_offset;
    while (total_bytes < size)
    {
        int xfer_stat;

        // Either a leading partial page or a full page or a fully contained page.
        // [....****] or [****....] or [********] or [..****..]
        unsigned xfer_size = start % bd->page_size ? bd->page_size - start : bd->page_size;

        // Clip size for trailing partial page or fully contained within one page.
        // [****....] or  [..****..]
        if (total_bytes + xfer_size > size) xfer_size = size - total_bytes;

        // Remember <start> applies to the device, total_bytes to <buf>.
        xfer_stat = read
                  ? read_dev_data( bd->i2c, bd->i2c_addr, start, bd->offset_size, buf + total_bytes, xfer_size)
                  : write_dev_data(bd->i2c, bd->i2c_addr, start, bd->offset_size, buf + total_bytes, xfer_size);

        if (xfer_stat == FAILURE)
        {
            PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "Failed to %s device 0x%02X", read ? "read from" : "write to", (unsigned) bd->i2c_addr);
            return FAILURE;
        }

        // Next page (if any)
        total_bytes += xfer_size;
        start       += xfer_size;
    }

    // Caller may want to check how many bytes we transferred.
    if (count) *count = total_bytes;

    return SUCCESS;
}

int i2c_read(i2c_bd *bd, void *buf, unsigned size, unsigned *count)
{
    return i2c_xfer(bd, buf, size, count, I2C_READ);
}

int i2c_write(i2c_bd *bd, void const *buf, unsigned size, unsigned *count)
{
    return i2c_xfer(bd, (void *) buf, size, count, I2C_WRITE);
}

void release_i2c_controller(ibmuftian_i2c *i2c)
{
    up(&i2c->i2c_lock);
}

static ssize_t attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    i2c_attr *i2c_attribute = to_i2c_attr(attr);
    ibmuftian_i2c *i2c = to_i2c(kobj);

    return i2c_attribute->show ? i2c_attribute->show(i2c, i2c_attribute, buf) : -EIO;
}

static ssize_t attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    i2c_attr *i2c_attribute = to_i2c_attr(attr);
    ibmuftian_i2c *i2c = to_i2c(kobj);

    return i2c_attribute->store ? i2c_attribute->store(i2c, i2c_attribute, buf, count) : -EIO;
}

static ssize_t i2c_show(ibmuftian_i2c *i2c, i2c_attr *attr, char *buf)
{
    int num_chars;

    if (unlikely(!i2c) || unlikely(!buf) || unlikely(!attr)) return 0;

    if (!strcmp(attr->attr.name, "speed"))
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%u kbps\n", i2c->speed);
    }
    else
    {
        num_chars = safe_sprintf(buf, PAGE_SIZE, "%s() - method for '%s' not yet implemented\n", __func__, attr->attr.name);
    }

    return num_chars;
}

int lock_i2c_controller(ibmuftian_i2c *i2c)
{
    if (down_interruptible(&i2c->i2c_lock))
    {
        PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "Interrupt while acquiring I2C bus semaphore, exiting");
        return -EINTR;
    }
	return SUCCESS;
}

static void write_ctrl(ibmuftian_i2c *i2c, int enable)
{
    ibmuftian_i2c_ctrl ctrl = { .i2c_ctrl = read_bar_addr(i2c->bar4, I2C_CTRL) };
    ctrl.bits.enable = enable ? 1 : 0;
    write_bar_addr(i2c->bar4, I2C_CTRL, ctrl.i2c_ctrl);
}

static void release(struct kobject *kobj)
{
    if (likely(kobj))
    {
        ibmuftian_i2c *i2c = to_i2c(kobj);
        ZERO_AND_FREE(i2c);
    }
}

static int set_freq_khz(ibmuftian_i2c *i2c, u32 khz)
{
    int status = FAILURE;

    write_ctrl(i2c, DISABLE);

    if (likely(i2c))
    {
        u32 prescale = PRESCALE(khz);
        ibmuftian_i2c_ctrl ctrl = { .i2c_ctrl = read_bar_addr(i2c->bar4, I2C_CTRL) };

        // Can't change the pre-scale while controller is enabled.
        if (ctrl.i2c_ctrl != (u32) -1 && !ctrl.bits.enable)
        {
            write_bar_addr(i2c->bar4, I2C_CLK_HI, (u8)(prescale >> 8));
            write_bar_addr(i2c->bar4, I2C_CLK_LO, (u8)(prescale >> 0));

            if (read_bar_addr(i2c->bar4, I2C_CLK_HI) == (u8)(prescale >> 8)
            &&  read_bar_addr(i2c->bar4, I2C_CLK_LO) == (u8)(prescale >> 0))
            {
                i2c->speed = khz;
                status = SUCCESS;
            }
        }
    }
    
    write_ctrl(i2c, ENABLE);

    return status;
}

void execute_i2c_command(ibmuftian_i2c *i2c)
{
    int const channel_codes[] = { CHANNEL_CODE };
    u8 chan_code = channel_codes[i2c->command.params[CMD_CHANNEL]];
    u32 read_buff = 0x0;
    u32 write_buff = 0x0;

    i2c_bd cmd_des =
    {
        .i2c = i2c,
        .i2c_addr = i2c->command.params[CMD_BASE_ADDR],
        .page_size = ONE_PAGE,
        .dev_offset = i2c->command.params[CMD_DEV_OFF],
        .offset_size = i2c->command.params[CMD_OFFSET_LEN]
    };

    i2c_bd mux_bd =
    {
        .i2c              = i2c,
        .i2c_addr         = I2C_MUX_CNT,
        .page_size        = ONE_PAGE,
        .dev_offset       = ZERO_DEV_OFFSET,
        .offset_size      = ZERO_OFFSET_SIZE
    };

    PRINT(DBG_PRAM({I2C_DBG}), "params %X %X %d %d %d", i2c->command.params[CMD_BASE_ADDR], i2c->command.params[CMD_DEV_OFF], i2c->command.params[CMD_OFFSET_LEN], i2c->command.params[CMD_CHANNEL], chan_code);
    
    if(i2c_write(&mux_bd, &chan_code, sizeof chan_code, NULL))
    {
        PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}),"Failed to set Mux to %d channel", i2c->command.params[CMD_CHANNEL]);
        safe_sprintf(i2c->command_result,I2C_BUFF_LEN,"setting channel failed");
    }

    if(i2c->command.command_type.cmd_type_int == I2C_READ)
    {
        if(i2c_read(&cmd_des, &read_buff, sizeof(read_buff), NULL))
        {
            PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "Failed to read buff %8.8x", cmd_des.i2c_addr);
            safe_sprintf(i2c->command_result, I2C_BUFF_LEN, "failed to read");
        }
        else
            safe_sprintf(i2c->command_result, I2C_BUFF_LEN, "%8.8x", read_buff);
    }
    else
    {
        write_buff = i2c->command.params[CMD_DATA];
        if(i2c_write(&cmd_des, &write_buff, sizeof(write_buff), NULL))
        {
            PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "Failed to write buff %8.8x", cmd_des.i2c_addr);
            safe_sprintf(i2c->command_result, I2C_BUFF_LEN, "failed to write");
        }
    }
}

int validate_i2c_cmd(ibmuftian_i2c *i2c)
{
    if(strcmp(i2c->command.command_type.cmd_type_str, "\0"))
    {
        if(strcmp(i2c->command.command_type.cmd_type_str, I2C_READ_CMD))
        {
            if(strcmp(i2c->command.command_type.cmd_type_str, I2C_WRITE_CMD))
            {
                return FAILURE;
            }
            else
                i2c->command.command_type.cmd_type_int = I2C_WRITE;
        }
        else
            i2c->command.command_type.cmd_type_int = I2C_READ;
    }
    else
        return FAILURE;

    return SUCCESS;
}

void parse_i2c_command(ibmuftian_i2c *i2c)
{
    char *cmd_ptr = i2c->command_buff;
    char *endptr, *parms;
    int params, channel;
    if(trim_str(cmd_ptr))
    {
        PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "Command string empty");
        safe_sprintf(i2c->command_result, I2C_BUFF_LEN, "no command");
    }

    PRINT(DBG_PRAM({I2C_DBG}), "The command is %s", cmd_ptr);

    parms = strsep(&cmd_ptr, " ");
    if(parms != NULL) strncpy(i2c->command.command_type.cmd_type_str, parms, CMD_TYPE_LEN);
    else strncpy(i2c->command.command_type.cmd_type_str, "\0", CMD_TYPE_LEN);

    if(validate_i2c_cmd(i2c))
    {
        PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "Invalid i2c command");
        safe_sprintf(i2c->command_result, I2C_BUFF_LEN, "invalid i2c command");
    }
    else
    {
        for(params = 0; params < I2C_NUM_PARAMS; params ++)
        {
            parms = strsep(&cmd_ptr, " ");
            if(params < (i2c->command.command_type.cmd_type_int == I2C_READ ? 2 : 3))
            {
                if(parms != NULL) i2c->command.params[params] = simple_strtol(parms, &endptr, HEX);
                else i2c->command.params[params] = 0;
            }
            else
            {
                if(params < CMD_OFFSET_LEN)
                    params = CMD_OFFSET_LEN;
                if(parms != NULL) i2c->command.params[params] = simple_strtol(parms, &endptr, DEC);
                else i2c->command.params[params] = 0;
            }
        }

        channel = --i2c->command.params[CMD_CHANNEL]; 
        if((channel >= 0) && (channel < 7))
            execute_i2c_command(i2c);        
    }
    
}

static ssize_t i2c_command_show(ibmuftian_i2c *i2c, i2c_attr *attr, char *buf)
{
    return safe_sprintf(buf, PAGE_SIZE, "%s\n", i2c->command_result);
}

static ssize_t i2c_command_store(ibmuftian_i2c *i2c, i2c_attr *attr, const char *buf, size_t count)
{
    char *cmd_buff = i2c->command_buff;
    if(count > I2C_COMMAND_LEN)
    {
        PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "size of command exceeds max length");
        return FAILURE;
    }

    strncpy(cmd_buff, buf, count);

    parse_i2c_command(i2c);
    return count;
}

int create_i2c(ibmuftian_board * associated_board)
{
    ibmuftian_i2c *new_i2c = NULL;
    static i2c_attr speed = __ATTR(speed , S_IRUGO, i2c_show, NULL);
    static i2c_attr i2c_command = __ATTR(i2c_command, S_IRUGO | S_IWUGO, i2c_command_show, i2c_command_store);

    static struct attribute *i2c_attrs[] =
    {
        &speed.attr,
        &i2c_command.attr,
        NULL
    };
    
    static struct sysfs_ops i2c_sysfs_ops = { .show  = attr_show, .store = attr_store };
    
    static struct kobj_type ktype =
    {
        .release        = release,
        .sysfs_ops      = &i2c_sysfs_ops,
        .default_attrs  = i2c_attrs,
    };
    
    PRINT(DBG_PRAM({I2C_DBG}), "creating i2c");

    PRINT(DBG_PRAM({I2C_DBG}), "allocating memory for i2c");

    new_i2c = kzalloc(sizeof(ibmuftian_i2c), GFP_KERNEL);
    if(unlikely(!new_i2c))
    {
        PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "failed to allocate memory for i2c");
        return FAILURE;
    }

    associated_board->i2c_cntrl = new_i2c;
    
    sema_init(&new_i2c->i2c_lock, 0);
	new_i2c->id = associated_board->id;
	new_i2c->bar4 = associated_board->bar4;
    safe_sprintf(new_i2c->command_result,I2C_BUFF_LEN-1,"no command");
	
	set_freq_khz(new_i2c, 100);
	
    PRINT(DBG_PRAM({I2C_DBG}), "i2c set up done");

    new_i2c->ready = 1;

	if (unlikely(kobject_init_and_add(&new_i2c->kobj, &ktype, &(associated_board->kobj), "i2c")))
    {
        PRINT(DBG_PRAM({I2C_DBG, ERR_DBG}), "kobject init/add failed for i2c");
        goto clean_exit;
    }

    up(&new_i2c->i2c_lock);
    return SUCCESS;

clean_exit:
	kobject_put(&new_i2c->kobj);
	associated_board->i2c_cntrl = NULL;
	return FAILURE;
}

void i2c_delete(ibmuftian_i2c *i2c)
{
    if(likely(i2c))
    {
        kobject_put(&i2c->kobj);
	}
}
