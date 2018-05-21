#include "ibmuftian_bdmgr.h"
#include "ibmuftian_engn.h"
#include "ibmuftian_fpga.h"
#include "ibmuftian.h"

void delete_status_buffer(ibmuftian_engn * engn, int bd_type)
{
    if(bd_type == IBD)
        dma_unmap_page(&engn->fpga->device_info.pdev->dev,engn->buff_desc.status_buffs[bd_type].status_buff_addr,PAGE_SIZE,DMA_TO_DEVICE);
    else
        dma_unmap_page(&engn->fpga->device_info.pdev->dev,engn->buff_desc.status_buffs[bd_type].status_buff_addr,PAGE_SIZE,DMA_FROM_DEVICE);

    clr_page_reserved(engn->buff_desc.status_buffs[bd_type].status_buff, get_order(PAGE_SIZE));
	__free_pages(engn->buff_desc.status_buffs[bd_type].status_buff, get_order(PAGE_SIZE));
    PRINT(DBG_PRAM({FULL_DBG}), "%s status buffer deleted", bd_type?"IBD":"OBD");
}

int init_status_buffer(ibmuftian_engn * engn, int bd_type)
{
    int loop_var;
    
    if(!(engn->buff_desc.status_buffs[bd_type].status_buff = alloc_pages(GFP_KERNEL, get_order(PAGE_SIZE)))) // page_size = 4096 = 32*dword*32//
    {
        PRINT(DBG_PRAM({BD_DBG, ERR_DBG}), "Failed to allocate memory for status buffer");
        return FAILURE;
    }
    
    set_page_reserved(engn->buff_desc.status_buffs[bd_type].status_buff, get_order(PAGE_SIZE));
    
    if(bd_type == IBD)
    {
        engn->buff_desc.status_buffs[bd_type].status_buff_addr = dma_map_page(&engn->fpga->device_info.pdev->dev,engn->buff_desc.status_buffs[bd_type].status_buff,0,PAGE_SIZE, DMA_TO_DEVICE);

        for(loop_var=0;loop_var<PAGE_SIZE;loop_var+=DWORD_LEN)
	        *((u32*)((char *)page_address(engn->buff_desc.status_buffs[bd_type].status_buff)+loop_var)) = DEADBEEF;

        write_bar_addr(engn->bar0, STATUS_IBD_LEN, STATUS_BUFF_LEN_DWORD * NUM_STATUS_BUFF);
        write_bar_addr(engn->bar0, STATUS_IBD_ADDR_HI, (u32)hi_addr(engn->buff_desc.status_buffs[bd_type].status_buff_addr));
        write_bar_addr(engn->bar0, STATUS_IBD_ADDR_LO, (u32)lo_addr(engn->buff_desc.status_buffs[bd_type].status_buff_addr));
    }
    else
    {
        engn->buff_desc.status_buffs[bd_type].status_buff_addr = dma_map_page(&engn->fpga->device_info.pdev->dev,engn->buff_desc.status_buffs[bd_type].status_buff,0,PAGE_SIZE, DMA_FROM_DEVICE);

        for(loop_var=0;loop_var<PAGE_SIZE;loop_var+=DWORD_LEN)
	        *((u32*)((char *)page_address(engn->buff_desc.status_buffs[bd_type].status_buff)+loop_var)) = DEADBEEF;

        write_bar_addr(engn->bar0, STATUS_OBD_LEN, STATUS_BUFF_LEN_DWORD * NUM_STATUS_BUFF);
        write_bar_addr(engn->bar0, STATUS_OBD_ADDR_HI, (u32)hi_addr(engn->buff_desc.status_buffs[bd_type].status_buff_addr));
        write_bar_addr(engn->bar0, STATUS_OBD_ADDR_LO, (u32)lo_addr(engn->buff_desc.status_buffs[bd_type].status_buff_addr));
    }

    PRINT(DBG_PRAM({FULL_DBG}), "%s status buffer initalised", bd_type?"IBD":"OBD");

    return SUCCESS;
}

int init_bd_mem(ibmuftian_engn *engn, int bd_type)
{
    if(!(engn->buff_desc.bd_add[bd_type].vir_add = dma_alloc_coherent(&(engn->fpga->device_info.pdev->dev), MAX_BD_SIZE * BD_FIFO_LEN, &(engn->buff_desc.bd_add[bd_type].dma_add), GFP_KERNEL | GFP_DMA32)))
    {
        PRINT(DBG_PRAM({BD_DBG, ERR_DBG}), "Failed to allocate memory for buffer descriptor");
        return FAILURE;
    }

    PRINT(DBG_PRAM({FULL_DBG}), "%s buffer descriptor memory initalized", bd_type?"IBD":"OBD");
    
    return SUCCESS;
}

void free_bd_mem(ibmuftian_engn *engn, int bd_type)
{
    if(likely(engn->buff_desc.bd_add[bd_type].vir_add))
    {
        dma_free_coherent(&engn->fpga->device_info.pdev->dev,  MAX_BD_SIZE * BD_FIFO_LEN, engn->buff_desc.bd_add[bd_type].vir_add, engn->buff_desc.bd_add[bd_type].dma_add);
        engn->buff_desc.bd_add[bd_type].vir_add = NULL;
    }

    PRINT(DBG_PRAM({FULL_DBG}), "%s buffer descriptor memory cleared", bd_type?"IBD":"OBD");
}

void delete_bd_fifo(ibmuftian_engn *engn, int bd_type)
{
    kfifo_free(&engn->buff_desc.bd_fifo[bd_type]);
    sema_init(&engn->buff_desc.bd_fifo_sema[bd_type], ZERO);
    PRINT(DBG_PRAM({FULL_DBG}), "%s buffer descriptor fifo deleted", bd_type?"IBD":"OBD");
}

int init_bd_fifo(ibmuftian_engn *engn, int bd_type)
{
    int loop_var;
    bd_addr mem_addr;
    
    if(kfifo_alloc(&engn->buff_desc.bd_fifo[bd_type], BD_FIFO_LEN * sizeof(bd_addr), GFP_KERNEL)) 
    {
        PRINT(DBG_PRAM({BD_DBG, ERR_DBG}), "Failed to allocate memory for buffer descriptor fifo for %s", bd_type==IBD?"IBD":"OBD");
        return FAILURE;
    }
    spin_lock_init(&engn->buff_desc.fifo_lock[bd_type]);
    for(loop_var = 0; loop_var < BD_FIFO_LEN; loop_var++)
    {
        mem_addr.vir_add = (((char *)engn->buff_desc.bd_add[bd_type].vir_add) + (loop_var * MAX_BD_SIZE));
        mem_addr.dma_add = ((engn->buff_desc.bd_add[bd_type].dma_add) + (loop_var * MAX_BD_SIZE));
        memset(mem_addr.vir_add, 0, sizeof(buffer_desc));
        kfifo_in_spinlocked(&engn->buff_desc.bd_fifo[bd_type], &mem_addr, sizeof(bd_addr), &engn->buff_desc.fifo_lock[bd_type]);
    }
    sema_init(&engn->buff_desc.bd_fifo_sema[bd_type], BD_FIFO_LEN - 1);
    
    PRINT(DBG_PRAM({FULL_DBG}), "%s buffer descriptor fifo initalized", bd_type?"IBD":"OBD");

    return SUCCESS;
    
}

void delete_bd_book_keeping(ibmuftian_engn *engn, int bd_type)
{
    delete_bd_fifo(engn, bd_type);
    free_bd_mem(engn, bd_type);
    delete_status_buffer(engn, bd_type);
}

int init_bd_book_keeping(ibmuftian_engn *engn, int bd_type)
{
    if(init_status_buffer(engn, bd_type)) return FAILURE;
    if(init_bd_mem(engn, bd_type)) goto cleanup_status_buff;
    if(init_bd_fifo(engn, bd_type)) goto cleanup_bd_mem;
    engn->buff_desc.bd_in_queue[bd_type] = 0;
    return SUCCESS;
    
    cleanup_bd_mem:
        free_bd_mem(engn, bd_type);
    cleanup_status_buff:
        delete_status_buffer(engn, bd_type);
        return FAILURE;
}

void reset_fifo_status_buffer(ibmuftian_engn * engn, int bd_type)
{
    int loop_var = 0;
    bd_addr mem_addr;
    unsigned long flags;

    for(loop_var=0;loop_var<PAGE_SIZE;loop_var+=DWORD_LEN)
        *((u32*)((char *)page_address(engn->buff_desc.status_buffs[bd_type].status_buff)+loop_var)) = DEADBEEF;

    spin_lock_irqsave(&engn->buff_desc.fifo_lock[bd_type], flags);
    kfifo_reset(&engn->buff_desc.bd_fifo[bd_type]);
    for(loop_var = 0; loop_var < BD_FIFO_LEN; loop_var++)
    {
        mem_addr.vir_add = (((char *)engn->buff_desc.bd_add[bd_type].vir_add) + (loop_var * MAX_BD_SIZE));
        mem_addr.dma_add = ((engn->buff_desc.bd_add[bd_type].dma_add) + (loop_var * MAX_BD_SIZE));
        memset(mem_addr.vir_add, 0, sizeof(buffer_desc));
        kfifo_in(&engn->buff_desc.bd_fifo[bd_type], &mem_addr, sizeof(bd_addr));
    }
    spin_unlock_irqrestore(&engn->buff_desc.fifo_lock[bd_type], flags);
    sema_init(&engn->buff_desc.bd_fifo_sema[bd_type], BD_FIFO_LEN - 1);
}

void get_bd(ibmuftian_engn *engn, int bd_type)
{
    down(&engn->buff_desc.bd_fifo_sema[bd_type]);
}

void return_bd(ibmuftian_engn *engn, int bd_type)
{
    up(&engn->buff_desc.bd_fifo_sema[bd_type]);
}

void reset_and_return_bd(ibmuftian_engn * engn, bd_bookeeping * bd_ptr)
{
    int bd_type = bd_ptr->bd_type == READ_MEM ? OBD : IBD;

    del_timer(&bd_ptr->bd_timer);
    engn->buff_desc.bd_in_queue[bd_type]--;
    memset(bd_ptr->bd_mem_ptr.vir_add, 0, sizeof(buffer_desc));
    kfifo_in_spinlocked(&engn->buff_desc.bd_fifo[bd_type], &bd_ptr->bd_mem_ptr, sizeof(bd_addr), &engn->buff_desc.fifo_lock[bd_type]);
    return_bd(engn, bd_type);
}

int build_send_bd(ibmuftian_engn *engn, ibmuftian_job *job)
{
    int loop_var = 0, gpr_loop_var = 0, ret = 0, for_var = 0;
    int bd_type = job->job_type == READ_MEM ? OBD : IBD;
    bd_bookeeping * bd_ptr = job->bd;
    buffer_desc * actual_bd;

    for(loop_var = 0; loop_var < job->total_bd; loop_var++)
    {
        unsigned long flags = 0;
        get_bd(engn, bd_type);
        ret = kfifo_out_spinlocked(&engn->buff_desc.bd_fifo[bd_type], &bd_ptr[loop_var].bd_mem_ptr, sizeof(bd_addr), &engn->buff_desc.fifo_lock[bd_type]);
        actual_bd = (buffer_desc *)bd_ptr[loop_var].bd_mem_ptr.vir_add;
        memset(actual_bd, 0, sizeof(buffer_desc));
        actual_bd->control_bits.bit.desc_id = bd_ptr[loop_var].bd_num;
        actual_bd->control_bits.bit.hmc_instq = bd_ptr[loop_var].inst_mem;
        actual_bd->control_bits.bit.desc_int_enable = bd_ptr[loop_var].set_interrupt;
        
        switch(job->job_type)
        {
            case INST:
            {
                if(bd_ptr[loop_var].inst_mem == INST_IOCTL)
                {
                    actual_bd->total_len = bd_ptr[loop_var].mem_addr.inst.len/DWORD_LEN + (bd_ptr[loop_var].mem_addr.inst.len%DWORD_LEN?1:0);
                    actual_bd->bd_mem_element.inst.src_host_addr_hi = (u32)hi_addr((dma_addr_t)bd_ptr[loop_var].inst_dma_addr);
                    actual_bd->bd_mem_element.inst.src_host_addr_lo = (u32)lo_addr((dma_addr_t)bd_ptr[loop_var].inst_dma_addr);
                    actual_bd->bd_mem_element.inst.gpr_mask = bd_ptr[loop_var].gprs.enable_mask;
                    for(gpr_loop_var = 0; gpr_loop_var < BD_MAX_GPR_REGS; gpr_loop_var++)
                    {
                        if(actual_bd->bd_mem_element.inst.gpr_mask & 1<<gpr_loop_var)
                        {
                            actual_bd->bd_mem_element.inst.gpr_reg[gpr_loop_var].gpr_val_low = *(u32*)((char*)bd_ptr[loop_var].gprs.reg_val + gpr_loop_var*GPR_LOW);
                            actual_bd->bd_mem_element.inst.gpr_reg[gpr_loop_var].gpr_val_high = *(u32*)((char*)bd_ptr[loop_var].gprs.reg_val + gpr_loop_var*GPR_HIGH);
                        }
                        else
                        {
                             actual_bd->bd_mem_element.inst.gpr_reg[gpr_loop_var].gpr_val_low = 0;
                             actual_bd->bd_mem_element.inst.gpr_reg[gpr_loop_var].gpr_val_high = 0;
                        }
                    }

                    bd_ptr[loop_var].size = INST_BD_SIZE;

                    actual_bd->control_bits.bit.length = INST_BD_SIZE/DWORD_LEN + (INST_BD_SIZE%DWORD_LEN?1:0);
                    break;
                }
            }

            case WRITE_MEM:
            {
                u32 bd_size = 0;
                int nents_loop_var = 0, host_addr_ptr = 0;
                dma_addr_t sg_data_start, host_addr;
                struct scatterlist * s_list;
                
                actual_bd->total_len = bd_ptr[loop_var].mem_addr.data.len/DWORD_LEN;
                actual_bd->bd_mem_element.mem_ibd.dst_hmc_addr_hi = (u32)hi_addr((dma_addr_t)bd_ptr[loop_var].mem_addr.data.dst);
                actual_bd->bd_mem_element.mem_ibd.dst_hmc_addr_lo = (u32)lo_addr((dma_addr_t)bd_ptr[loop_var].mem_addr.data.dst);

                bd_size += BASE_MEM_BD_LEN;
                s_list = bd_ptr[loop_var].sg_data->sgl;
                
                while(nents_loop_var < bd_ptr[loop_var].sg_data->nents)
                {
                    sg_data_start = sg_dma_address(s_list);
                    host_addr = sg_data_start;
                    
                    while(host_addr < sg_data_start + sg_dma_len(s_list))
                    {
                        actual_bd->bd_mem_element.mem_ibd.src_addr[host_addr_ptr].src_host_addr_hi = (u32)hi_addr(host_addr);
                        actual_bd->bd_mem_element.mem_ibd.src_addr[host_addr_ptr].src_host_addr_lo = (u32)lo_addr(host_addr);

                        host_addr_ptr++;
                        
                        if(host_addr == sg_data_start && nents_loop_var == ZERO)
                            host_addr += DMA_DATA_SIZE - sg_data_start%DMA_DATA_SIZE;
                        else
                            host_addr += DMA_DATA_SIZE;

                        bd_size += ADDR_LEN;
                    }

                    nents_loop_var++;
                    s_list = sg_next(s_list);
                }
                
                bd_ptr[loop_var].size = bd_size;

                actual_bd->control_bits.bit.length = (u32)((bd_size/DWORD_LEN) + (bd_size%DWORD_LEN?1:0));

                break;
            }

            case READ_MEM:
            {
                
                u32 bd_size = 0;
                int nents_loop_var = 0, host_addr_ptr = 0;
                dma_addr_t sg_data_start, host_addr;
                struct scatterlist * s_list;
                
                actual_bd->total_len = bd_ptr[loop_var].mem_addr.data.len/DWORD_LEN;
                actual_bd->bd_mem_element.mem_obd.src_hmc_addr_hi = (u32)hi_addr((dma_addr_t)bd_ptr[loop_var].mem_addr.data.src);
                actual_bd->bd_mem_element.mem_obd.src_hmc_addr_lo = (u32)lo_addr((dma_addr_t)bd_ptr[loop_var].mem_addr.data.src);
                
                bd_size += BASE_MEM_BD_LEN;
                s_list = bd_ptr[loop_var].sg_data->sgl;
                while(nents_loop_var < bd_ptr[loop_var].sg_data->nents)
                {
                    sg_data_start = sg_dma_address(s_list);
                    host_addr = sg_data_start;
                    while(host_addr < sg_data_start + sg_dma_len(s_list))
                    {
                        actual_bd->bd_mem_element.mem_obd.dest_addr[host_addr_ptr].dst_host_addr_hi = (u32)hi_addr(host_addr);
                        actual_bd->bd_mem_element.mem_obd.dest_addr[host_addr_ptr].dst_host_addr_lo = (u32)lo_addr(host_addr);

                        host_addr_ptr++;
                        
                        if(host_addr == sg_data_start && nents_loop_var == ZERO)
                            host_addr += DMA_DATA_SIZE - sg_data_start%DMA_DATA_SIZE;
                        else
                            host_addr += DMA_DATA_SIZE;

                        bd_size += ADDR_LEN;
                    }

                    nents_loop_var++;
                    s_list = sg_next(s_list);
                }

                bd_ptr[loop_var].size = bd_size;

                actual_bd->control_bits.bit.length = bd_size/DWORD_LEN + (bd_size%DWORD_LEN?1:0);
                break;
            }
        }

        bd_ptr[loop_var].bd_len.bit.length = bd_ptr[loop_var].size/DWORD_LEN;
        
        if((loop_var == BD_IN_INST_IOCTL - INST_BD_OFFSET) && (job->job_type == INST))
        {
            wait_for_completion(&bd_ptr[loop_var- 1].inst_bookeeping.ibd_completion);
        }
        
        if(debug_areas[FULL_DBG])
        {
            PRINT(DBG_PRAM({IOCTL_DBG}),"BD DESCRIPTOR CONTENTS:\n");
            for(for_var=0; for_var< actual_bd->control_bits.bit.length; for_var++)    
                PRINT(DBG_PRAM({IOCTL_DBG}),"DW%d: 0x%8.8X", for_var, *(u32*)((void *)actual_bd + for_var*4));
            PRINT(DBG_PRAM({IOCTL_DBG}),"--------------------------------------------\n\n");
        }
        
        spin_lock_irqsave(&engn->bar0->bar_lock, flags);
        if(bd_ptr[loop_var].bd_type == INST)
            mod_timer(&(bd_ptr[loop_var].bd_timer), jiffies + INST_BD_TIMEOUT);
        else
            mod_timer(&(bd_ptr[loop_var].bd_timer), jiffies + MEM_BD_TIMEOUT);

        engn->buff_desc.bd_in_queue[bd_type]++;

        if(job->job_type == INST          &&
           loop_var == (job->total_bd -1) &&
           !engn->buff_desc.inst_job++)
        {
            do_gettimeofday(&engn->duration);

            if(ibmuftian.functions.engn_inuse)
                ibmuftian.functions.engn_inuse(engn->id +(engn->fpga->engns.engn_count * engn->fpga->id), 1);
        }

        if(bd_type == IBD)
        {
            write_bar_addr(engn->bar0, IBD_LEN_ADDR, bd_ptr[loop_var].bd_len.bit.length);
			write_bar_addr(engn->bar0, IBD_ADDR_HI, (u32)hi_addr(bd_ptr[loop_var].bd_mem_ptr.dma_add));
			write_bar_addr(engn->bar0, IBD_ADDR_LO, (u32)lo_addr(bd_ptr[loop_var].bd_mem_ptr.dma_add));
        }
        else
        {
            write_bar_addr(engn->bar0, OBD_LEN_ADDR, bd_ptr[loop_var].bd_len.bit.length);
			write_bar_addr(engn->bar0, OBD_ADDR_HI, (u32)hi_addr(bd_ptr[loop_var].bd_mem_ptr.dma_add));
			write_bar_addr(engn->bar0, OBD_ADDR_LO, (u32)lo_addr(bd_ptr[loop_var].bd_mem_ptr.dma_add));
        }

        spin_unlock_irqrestore(&engn->bar0->bar_lock, flags);

    }

    return SUCCESS;
}

void bd_timed_out(unsigned long arg)
{
    bd_bookeeping * bd_ptr = (bd_bookeeping *)arg;

    ibmuftian_job * job = (ibmuftian_job *)bd_ptr->parent_job;
    PRINT(DBG_PRAM({BD_DBG, ERR_DBG}), "buffer descriptor number %d timed out retrying the %d job", bd_ptr->bd_num, job->job_type);

    job->redo = REDO;
}

void bd_status_ckeck_cleanup(ibmuftian_engn * engn, bd_result * result, int bd_type)
{
    int bd_tag_id = 0, loop_var = 0, status_dword_loop_var = 0;
    ibmuftian_job * job = NULL;
    bd_bookeeping * bd_ptr = NULL;
    
    for(loop_var = 0; loop_var < NUM_STATUS_BUFF; loop_var++)
    {
        if(engn->aborted == 1)
        {
            return;
        }

        if(result[loop_var].buff_desc_result.dword[STATUS_BUFF_LEN_DWORD - 1] == FEEDFACE)
        {
            bd_tag_id = result[loop_var].buff_desc_result.dword_distribution.ibd_id;
            PRINT(DBG_PRAM({BD_DBG}), "kernel timer up %d low %d", result[loop_var].buff_desc_result.dword_distribution.timer_up, result[loop_var].buff_desc_result.dword_distribution.timer_low);
            for(status_dword_loop_var = 0; status_dword_loop_var < STATUS_BUFF_LEN_DWORD; status_dword_loop_var++)
            {
                result[loop_var].buff_desc_result.dword[status_dword_loop_var] = DEADBEEF;
            }
        }

        if(bd_tag_id)
        {
            spin_lock_bh(&engn->jobs.job_queue_lock);
            list_for_each_entry(job, &(engn->jobs.job_queue), list)
            {
                if(bd_tag_id >= job->bd_tag_start_id && bd_tag_id <= (job->bd_tag_start_id + job->total_bd -1))
                {
                    bd_ptr = &job->bd[bd_tag_id - job->bd_tag_start_id];
                    break;
                }
            }
    
            if(bd_ptr)
            {
                if(bd_ptr->bd_num == bd_tag_id)
                {
                    del_timer(&bd_ptr->bd_timer);
                    if(bd_ptr->inst_bookeeping.complete)
                    {
                        complete_all(&(bd_ptr->inst_bookeeping.ibd_completion));
                    }
                    reset_and_return_bd(engn, bd_ptr);
                    bd_ptr->done = DONE;

                    if(atomic_dec_and_test(&job->bd_count))
                    {
                        if(job->job_type == INST       &&
                           !--engn->buff_desc.inst_job)
                        {
                            struct timeval curr_time;
                            do_gettimeofday(&curr_time);
                            engn->engn_utilized.tv_sec += curr_time.tv_sec - engn->duration.tv_sec;
                            engn->engn_utilized.tv_usec += curr_time.tv_usec - engn->duration.tv_usec;
                            engn->duration.tv_sec = 0;
                            engn->duration.tv_usec = 0;

                            if(ibmuftian.functions.engn_inuse)
                                ibmuftian.functions.engn_inuse(engn->id +(engn->fpga->engns.engn_count * engn->fpga->id), 0);
                        }

                        list_del(&job->list);
                        complete_all(&job->job_completion);
                    }
                }
            }
            else
            {
                PRINT(DBG_PRAM({BD_DBG, ERR_DBG}), "BD id %d not found in any jobs", bd_tag_id);
                list_for_each_entry(job, &(engn->jobs.job_queue), list)
                {
                    PRINT(DBG_PRAM({BD_DBG, ERR_DBG}), "job start id %d and len %d",job->bd_tag_start_id,job->total_bd);
                }
            }
            spin_unlock_bh(&engn->jobs.job_queue_lock);
        }
        bd_tag_id = ZERO;
    }
    
    if(engn->buff_desc.bd_in_queue[bd_type] > ZERO)
    {
        if(bd_type == OBD)
            mod_timer(&engn->bar0->interrupt_service.obd_comp_timer, (jiffies + msecs_to_jiffies(1)));
        else
            mod_timer(&engn->bar0->interrupt_service.ibd_comp_timer, (jiffies + msecs_to_jiffies(1)));            
    }

    return;
}
