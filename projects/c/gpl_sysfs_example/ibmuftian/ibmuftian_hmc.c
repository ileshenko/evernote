#include "ibmuftian_hmc.h"
#include "ibmuftian.h"

int trainHMC(ibmuftian_bar * bar, ibmuftian_i2c * i2c, int hmc)
{
    int status = SUCCESS;
    u32 read_buff = 0x0;
    u32 read_reset = 0x0;
    u8 chan_code = I2C_MUX_CHAN_1;
    u8 f_h = 0;
    unsigned data_in = 0x0;
    unsigned bar4_add = BAR4_HMC_BASE + (hmc << BAR4_ADD_CALC_SHIFT);
    unsigned count = 0;

    i2c_bd mux_bd =
    {
        .i2c              = i2c,
        .i2c_addr         = I2C_MUX_CNT,
        .page_size        = ONE_PAGE,
        .dev_offset       = ZERO_DEV_OFFSET,
        .offset_size      = ZERO_OFFSET_SIZE
    };

    i2c_bd hmc_bd =
    {
        .i2c              = i2c,
        .i2c_addr         = I2C_HMC_BASE + hmc,
        .page_size        = FOUR_PAGES,
        .dev_offset       = ZERO_DEV_OFFSET,
        .offset_size      = HMC_OFFSET_SIZE
    };

    hmc_bd.i2c_addr = I2C_HMC_BASE + hmc;
    PRINT(DBG_PRAM({HMC_DBG}), "THE I2C ADDRESS IS %8.8x", hmc_bd.i2c_addr);

    lock_i2c_controller(i2c);

    write_bar_addr(bar,FPGA_I2C_ACCESS);
    PRINT(DBG_PRAM({HMC_DBG}), "bar4wr %8.8x %8.8x",FPGA_I2C_ACCESS);

    if(i2c_write(&mux_bd, &chan_code, sizeof chan_code, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed to set Mux to HMC channel");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", mux_bd.i2c_addr, mux_bd.dev_offset, chan_code);

    read_buff = read_bar_addr(bar, FERR_SIGNALS_LOC); //check for fatal errors
    if(read_buff & FATAL_ERRORS)
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}), "fatal errors detected at the start");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "bar4rd %8.8x", FERR_SIGNALS_LOC);
    PRINT(DBG_PRAM({HMC_DBG}), "FERR REG %8.8x",read_buff);

    hmc_bd.dev_offset = GLOBAL_STATUS_REGISTER; // Check global status register
    if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}), "Failed to read global status register");
        status = FAILURE;
    }
    if(!(read_buff & LINK2_ACTIVE))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"link 2 not active");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
    PRINT(DBG_PRAM({HMC_DBG}), "GLOBAL STATUS REG %8.8x",read_buff);

    hmc_bd.dev_offset = LINK2_HMC_BASE + HMC_LINK_STATUS_OFFSET; //Link status check
    if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed to read link status register");
        status = FAILURE;
    }
    if(!(read_buff & LINK2_RESET))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}), "link 2 not in reset");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
    PRINT(DBG_PRAM({HMC_DBG}), "HMC LINK STATUS %8.8x", read_buff);

    hmc_bd.dev_offset = LINK2_HMC_BASE + HMC_LINK_SYNC_STATUS_OFFSET; //Check link sync status
    if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}), "Failed to read link sync status");
        status = FAILURE;
    }

    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
    PRINT(DBG_PRAM({HMC_DBG}), "HMC LINK SYNC STATUS %8.8x",read_buff);

    read_reset = read_bar_addr(bar, HMC_RESET_ADD); //check current status of reset registers
    PRINT(DBG_PRAM({HMC_DBG}), "bar4rd %8.8x", HMC_RESET_ADD);
    PRINT(DBG_PRAM({HMC_DBG}),"HMC RESET %8.8x",read_reset);

    if(!((read_reset & (1 << (hmc + 8))) && !(read_reset & (1 << hmc))))
    {
        PRINT(DBG_PRAM({HMC_DBG}), "ALREADY POST TRAINING CONFIG CHECKING IF TRAINED");
        hmc_bd.dev_offset = LINK2_HMC_BASE + HMC_LINK_STATUS_OFFSET; //Link status check
        if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
        {
            PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed to read link status register");
            status = FAILURE;
        }
        PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
        PRINT(DBG_PRAM({HMC_DBG}), "HMC LINK STATUS %8.8x", read_buff);

        if(!(read_buff & LINK2_TRAINED))
        {
            PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}), "HMC NOT TRAINED");
            read_reset &= ~(1 << (hmc));
            read_reset |= (1 << (hmc + 8));
            write_bar_addr(bar, HMC_RESET_ADD, read_reset);
            PRINT(DBG_PRAM({HMC_DBG}), "bar4wr %8.8x %8.8x", HMC_RESET_ADD,read_reset); 
        }
        else
        {
            PRINT(DBG_PRAM({HMC_DBG}),"HMC %d ALREADY TRAINED",hmc);
            release_i2c_controller(i2c);
            return SUCCESS;
        }
    }

    write_bar_addr(bar, HMC_RESET_ADD, read_reset |= (1 << hmc));// HMC controller out of reset
    PRINT(DBG_PRAM({HMC_DBG}), "bar4wr %8.8x %8.8x", HMC_RESET_ADD, read_reset | (1 << hmc)); 

    udelay(100);	

    write_bar_addr(bar, HMC_RESET_ADD, read_reset &= ~(1 << (hmc + 8)));// HMC controller get the control of HMC reset
    PRINT(DBG_PRAM({HMC_DBG}), "bar4wr %8.8x %8.8x", HMC_RESET_ADD, read_reset & ~(1 << (hmc + 8)));

    udelay(100);	

    write_bar_addr(bar, bar4_add, RESET_HMC_CONT_TX);
    PRINT(DBG_PRAM({HMC_DBG}), "bar4wr %8.8x %8.8x",bar4_add,RESET_HMC_CONT_TX); 

    udelay(100);	

    write_bar_addr(bar, bar4_add, 0x0); //HMC controller tx out of reset
    PRINT(DBG_PRAM({HMC_DBG}), "bar4wr %8.8x %8.8x",bar4_add, 0x0); 

    read_buff = 0x0;
    count = 0;
    while(!(read_buff & 0x2))
    { 
        read_buff = read_bar_addr(bar, bar4_add + HMC_CONTROLLER_SYS_STATUS_OFFSET);
        mdelay(0.02); 
        count++;
        if(count == 20)
            break; //TODO Discuss and change this if needed.
    }
    if(read_buff & (1 << F_H_READ))
        f_h = 1;
    PRINT(DBG_PRAM({HMC_DBG}), "bar4rd %8.8x", bar4_add + HMC_CONTROLLER_SYS_STATUS_OFFSET);
    PRINT(DBG_PRAM({HMC_DBG}), "HMC CONTROLLER SYS STATUS STATUS %8.8x",read_buff);

    read_buff = read_bar_addr(bar, bar4_add + HMC_CONTROLLER_STATE_MACHINE_OFFSET);

    PRINT(DBG_PRAM({HMC_DBG}), "bar4rd %8.8x", bar4_add + HMC_CONTROLLER_STATE_MACHINE_OFFSET);
    PRINT(DBG_PRAM({HMC_DBG}),"HMC STATE MACHINE %8.8x",read_buff);

    mdelay(2);

    hmc_bd.dev_offset = LINK2_HMC_BASE + LINK_RUN_LENGHT_LIM_REG_OFFSET;
    data_in = LINK_RUN_LENGTH_LIMIT;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 1 of setup");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = LINK0_HMC_BASE;
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    { 
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 2 of setup");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = LINK1_HMC_BASE; 
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 3 of setup");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = LINK3_HMC_BASE;
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 4 of setup");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = LINK2_HMC_BASE;
    data_in = LINK_CONFIG_IN_DEBUG;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 5 of setup");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = BUFFER_TOKEN_COUNT_REG;
    data_in = BUFFER_SPACE_AVAIL ;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 6 of setup");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = VALUE_CONTROL_REG;
    data_in = INIT_DEBUG_VALUE_CONTROL;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 7 of setup");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIDATA0;
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 1 of Link config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIDATA1;
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 2 of Link config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIDATA2;
    data_in = f_h ? 0x0 : 1 << F_H_SET;
    PRINT(DBG_PRAM({HMC_DBG}),"data in %8.8x",data_in);
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 3 of Link config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIDATA3;
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 4 of Link config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIREQ;
    data_in = INIT_LINK_CONFIG;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 5 of Link config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    read_buff = 0xFFFFFFFF;
    count = 0; 
    while(read_buff != 0x0) 
    {
        mdelay(0.2);
        if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
        {
            PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed read Link config buffer");
            status = FAILURE;
        }
        count++;
        if(count == 20)
            break; //TODO Discuss and change this if needed.
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
    PRINT(DBG_PRAM({HMC_DBG}), "link ERI %8.8x",read_buff);

    hmc_bd.dev_offset = ERIDATA0;
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 1 of Phy config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIDATA1;
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 2 of Phy config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIDATA2;
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 3 of Phy config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIDATA3;
    data_in = 0x0;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in , NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 4 of Phy config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIREQ;
    data_in = INIT_PHY_CONFIG;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in , NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed at step 5 of Phy config ERI");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    read_buff = 0xFFFFFFFF;
    count = 0;
    while(read_buff != 0x0)
    {
        mdelay(0.2);
        if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
        {
            PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed read phy config buffer");
            status = FAILURE;
        }
        count++;
        if(count == 20)
            break; //TODO Discuss and change this if needed.
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);  
    PRINT(DBG_PRAM({HMC_DBG}),"link PHY %8.8x",read_buff);

    hmc_bd.dev_offset = NVM_ADD;
    data_in = DISABLE_NVM_WR_STATUS;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed to disable nvm write status");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = REQ_IDENT_REG;
    if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}), "Failed to read link sync status");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
    PRINT(DBG_PRAM({HMC_DBG}), "THE CUBE ID IS %8.8x",read_buff);

    data_in = read_buff & CLEAR_CID;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed to disable nvm write status");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);

    hmc_bd.dev_offset = ERIREQ;
    data_in = BEGIN_LINK_TRAINING_ON_HMC;
    if (i2c_write(&hmc_bd, &data_in, sizeof data_in, NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed to begin link training");
        status = FAILURE;
    }
    write_bar_addr(bar, bar4_add + LINK_TRAINING_FPGA_OFFSET, LINK_TRAINING_FPGA); //init link training on fpga side
    PRINT(DBG_PRAM({HMC_DBG}), "i2cwr %8.8x %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset, data_in);
    PRINT(DBG_PRAM({HMC_DBG}), "bar4wr %8.8x %8.8x",bar4_add + LINK_TRAINING_FPGA_OFFSET, LINK_TRAINING_FPGA);

    read_buff = 0x80000000;
    count = 0;
    while(read_buff & 0x80000000)
    {
        udelay(20);
        if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
        {
            PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed read phy config buffer");
            status = FAILURE;
        }
        count++;
        if(count == 50000)
            break; //TODO Discuss and change this if needed.
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
    PRINT(DBG_PRAM({HMC_DBG}), "ERIREQ AFTER LINK TRAINIG %8.8x", read_buff);

    if(read_buff & tINITCONTI_VIOLATION) //TODO take care of all errors
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}), "timing wrong link training failed");
        status = FAILURE;
    } 

    hmc_bd.dev_offset = GLOBAL_STATUS_REGISTER; // Check global status register
    if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed to read global status buffer");
        status = FAILURE;
    }
    if(!(read_buff & LINK2_ACTIVE))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"link not active after training");
        status = FAILURE;
    } 
    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
    PRINT(DBG_PRAM({HMC_DBG}),"GLOBAL STATUS REG %8.8x", read_buff);

    hmc_bd.dev_offset = LINK2_HMC_BASE + HMC_LINK_STATUS_OFFSET; //Link status check
    if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed to read link status buffer");
        status = FAILURE;
    }
    if(!(read_buff & (f_h ? LINK_ACTIVE_FULLWIDTH : LINK_ACTIVE_HALFWIDTH)))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"not trained link something wrong "); 
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
    PRINT(DBG_PRAM({HMC_DBG}),"HMC LINK STATUS %8.8x", read_buff);

    hmc_bd.dev_offset = LINK2_HMC_BASE + HMC_LINK_SYNC_STATUS_OFFSET; //Check link sync status 
    if(i2c_read(&hmc_bd, &read_buff, sizeof(read_buff), NULL))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"Failed to read link sync status buffer");
        status = FAILURE;
    }
    if(!(read_buff & (f_h ? 0xFFFF0000 : 0x00FF0000)))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}),"link sync status not accurate"); 
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "i2crd %8.8x %8.8x", hmc_bd.i2c_addr, hmc_bd.dev_offset);
    PRINT(DBG_PRAM({HMC_DBG}),"HMC LINK SYNC STATUS %8.8x", read_buff);

    write_bar_addr(bar, bar4_add + TX_TOKEN_OFFSET , 0x200);// init token to 512 for operation.
    PRINT(DBG_PRAM({HMC_DBG}), "bar4wr %8.8x %8.8x",bar4_add + TX_TOKEN_OFFSET , 0x200);

    read_buff = 0x0;
    count = 0;
    while(!(read_buff & 0x4))
    { 
        read_buff = read_bar_addr(bar, bar4_add + HMC_CONTROLLER_SYS_STATUS_OFFSET);
        mdelay(0.02);
        count++;
        if(count == 20)
            break;
    }
    if(!(read_buff & 0x4))
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}), "INCORRECT HMC SYS STATUS");
    }
    PRINT(DBG_PRAM({HMC_DBG}), "bar4rd %8.8x", bar4_add + HMC_CONTROLLER_SYS_STATUS_OFFSET);
    PRINT(DBG_PRAM({HMC_DBG}),"HMC CONTROLLER SYS STATUS  %8.8x",read_buff);


    read_buff = read_bar_addr(bar, LINKUP_REG_ADD);
    write_bar_addr(bar, LINKUP_REG_ADD , read_buff |= (1 << hmc));//Enable DMA
    PRINT(DBG_PRAM({HMC_DBG}), "bar4wr %8.8x %8.8x",LINKUP_REG_ADD , read_buff);

    read_buff = read_bar_addr(bar, FERR_SIGNALS_LOC); //check for fatal errors
    if(read_buff & FATAL_ERRORS)
    {
        PRINT(DBG_PRAM({HMC_DBG, ERR_DBG}), "fatal errors detected at the end");
        status = FAILURE;
    }
    PRINT(DBG_PRAM({HMC_DBG}), "bar4rd %8.8x", FERR_SIGNALS_LOC);
    PRINT(DBG_PRAM({HMC_DBG}), "FERR REG %8.8x",read_buff);

    release_i2c_controller(i2c);
    return status;

}

