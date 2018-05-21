#include "ibmuftian_ioctl.h"
#include "ibmuftian.h"

int setup_job(ibmuftian_fpga *fpga, void *ioctl_data, unsigned type)
{
    ibmuftian_job * job = kzalloc(sizeof(ibmuftian_job),GFP_KERNEL);
    mem_buff *mem_ioctl = NULL;
    ioctl_inst *inst_ioctl = NULL;
    ibmuftian_engn * engn = NULL;
    int inst_bd = 0, data_bd = 0, total_bd = 0,
        inst_malloc = 0, data_malloc = 0, inst_pg_num = 0,
        data_pg_num = 0, max_bd = 0, loop_var = 0, ret = SUCCESS, dirtied = 0, err = 0, buf_id = 0;
    unsigned long max_data_size;
    sgt_container *sgl_data;
    dma_addr_t inst_dma_addr = 0;
    enum dma_data_direction direction = DMA_BIDIRECTIONAL; //initialization.
    struct page **inst_pgs, **data_pgs;
    void *inst_kernel_ptr, *data_kernel_ptr;
    bd_bookeeping *bd_ptr;
    
    if(type == INST)
    {
        PRINT(DBG_PRAM({IOCTL_DBG}), "inst ioctl");
        inst_ioctl = (ioctl_inst *)ioctl_data;
        mem_ioctl = &inst_ioctl->data;
        job->engn_no = inst_ioctl->eng_num;
    }
    else
    {
        PRINT(DBG_PRAM({IOCTL_DBG}), "data ioctl");
        mem_ioctl = (mem_buff *)ioctl_data;
        job->engn_no = 0;
    }

    
    switch(type)
    {
        case(INST):
        {
            inst_bd = 1;
            if((inst_dma_addr = map_user_memory(fpga->device_info.pdev, 
                                                 (unsigned long) inst_ioctl->inst.host_ptr, 
                                                 (unsigned long) (inst_ioctl->inst.host_ptr + inst_ioctl->inst.buff_size),
                                                 DMA_TO_DEVICE,
                                                 &inst_malloc,
                                                 &inst_pgs,
                                                 &inst_pg_num,
                                                 &inst_kernel_ptr)) == FAILURE)
            {
                PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "map user memory failed"); 
                return -ENOMEM;
            }
            
        }

        case(WRITE_MEM):
        {
            direction = DMA_TO_DEVICE;
            break;
        }

        case(READ_MEM):
        {
            direction = DMA_FROM_DEVICE;
            dirtied = DIRTY;
            break;
        }
    }
    max_data_size = MAX_DATA_SEGS*DMA_DATA_SIZE;

    if(fpga->fpga_info.bit.board_type == FIN)
        max_bd = TOTAL_FIN_MEM_SIZE/max_data_size;
    else if(fpga->fpga_info.bit.board_type == WRASSE)
        max_bd = TOTAL_WRASSE_MEM_SIZE/max_data_size;
    if((data_bd = map_user_pages(&sgl_data, 
                                  (unsigned long) mem_ioctl->host_ptr,
                                  (unsigned long)(mem_ioctl->host_ptr + mem_ioctl->buff_size),
                                  direction,
                                  max_data_size,
                                  &data_malloc,
                                  &max_bd,
                                  &data_pgs,
                                  &data_pg_num,
                                  &data_kernel_ptr)) == FAILURE)
    {
        PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "map user pages failed");
        return -ENOMEM;
    }

    total_bd = data_bd + inst_bd;
    PRINT(DBG_PRAM({IOCTL_DBG}), "data bd = %d inst bd %d", data_bd, inst_bd);
    job->pdev = fpga->device_info.pdev;
    job->total_bd = total_bd;
    job->job_type = type;
    init_completion(&(job->job_completion));
    atomic_set(&job->bd_count, total_bd);
    job->bd = kzalloc(sizeof(bd_bookeeping)*total_bd, GFP_KERNEL);
    memset(job->bd, ZERO, total_bd * sizeof(bd_bookeeping));
    bd_ptr = job->bd;
    engn = fpga->engns.engines[job->engn_no];

    if(engn->aborted)
    {
        PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "Resetting engine from previous abort");
        reset_aborted_engn(engn);
        engn->aborted = 0;
    }

    PRINT(DBG_PRAM({IOCTL_DBG}), "engn id = %d totalbd %d", engn->id, total_bd);
    
    spin_lock(&engn->id_lock);
    if(((unsigned short)(engn->bd_id + total_bd) < engn->bd_id))
        engn->bd_id = 0;

    buf_id = engn->bd_id;
    engn->bd_id = engn->bd_id + total_bd;
    spin_unlock(&engn->id_lock);

    for(loop_var = 0;loop_var < total_bd; loop_var++)
    {
        bd_ptr[loop_var].bd_num = ++buf_id;

        if(!loop_var)
            job->bd_tag_start_id = bd_ptr[loop_var].bd_num;

        bd_ptr[loop_var].bd_state = INIT;

        if((loop_var == BD_IN_INST_IOCTL - LAST_MEM_BD) && inst_bd)
        {
            init_completion((&bd_ptr[loop_var].inst_bookeeping.ibd_completion));
            bd_ptr[loop_var].set_interrupt = SET_INTERRUPT;
            bd_ptr[loop_var].inst_bookeeping.complete = 1;
        }
        
        if(!(bd_ptr[loop_var].bd_num % INTERRUPT_ENABLE) || (loop_var == total_bd - 1))
            bd_ptr[loop_var].set_interrupt = SET_INTERRUPT;
        bd_ptr[loop_var].parent_job = &job;

        bd_ptr[loop_var].bd_type = type;        

        if((loop_var == (total_bd-1)) && inst_bd)
        {
            bd_ptr[loop_var].inst_mem = INST_IOCTL;
            bd_ptr[loop_var].inst_dma_addr = inst_dma_addr;
            bd_ptr[loop_var].mem_addr.inst.count = inst_bd;
            bd_ptr[loop_var].mem_addr.inst.len = inst_ioctl->inst.buff_size;
            bd_ptr[loop_var].mem_addr.inst.offset = 0;
            bd_ptr[loop_var].mem_addr.inst.src = NULL;
            bd_ptr[loop_var].mem_addr.inst.dst = inst_ioctl->inst.host_ptr;
        }
        else
        {
            bd_ptr[loop_var].inst_mem = MEM_IOCTL;
            bd_ptr[loop_var].sg_data = sgl_data[loop_var].sgt;
            bd_ptr[loop_var].mem_addr.data.count = data_bd;
            bd_ptr[loop_var].mem_addr.data.offset = 0;
            bd_ptr[loop_var].mem_addr.data.len = sgl_data[loop_var].sgt_len;

            if(type == READ_MEM)
            {
                bd_ptr[loop_var].mem_addr.data.src = (void *)(mem_ioctl->uta_ptr + sgl_data[loop_var].sgt_offset);
                bd_ptr[loop_var].mem_addr.data.dst = NULL;
                if((ret = dma_map_sg(&fpga->device_info.pdev->dev,
                                      bd_ptr[loop_var].sg_data->sgl,
                                      bd_ptr[loop_var].sg_data->nents,
              		                  DMA_FROM_DEVICE)) <= 0)
                {
                    PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "dma map sg failed");
                    return ret;
                }
            }
            else 
            {
                bd_ptr[loop_var].mem_addr.data.dst = (void *)(mem_ioctl->uta_ptr + sgl_data[loop_var].sgt_offset);
                bd_ptr[loop_var].mem_addr.data.src = NULL;
                if((ret = dma_map_sg(&fpga->device_info.pdev->dev,
                                      bd_ptr[loop_var].sg_data->sgl,
                                      bd_ptr[loop_var].sg_data->nents,
              		                  DMA_TO_DEVICE)) <= 0)
                {
                    PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "dma map sg failed");
                    return ret;
                }
            }
        }
        if(type == INST)
        {
            bd_ptr[loop_var].gprs.enable_mask = inst_ioctl->reg_init.enable_mask;
            bd_ptr[loop_var].gprs.reg_val = inst_ioctl->reg_init.reg_val;
        }

        bd_ptr[loop_var].done = IN_PROGRESS;
        setup_timer(&(bd_ptr[loop_var].bd_timer), &bd_timed_out, (unsigned long)&bd_ptr[loop_var]);
    }

    spin_lock_bh(&engn->jobs.job_queue_lock);
    list_add(&(job->list), &(engn->jobs.job_queue));
    spin_unlock_bh(&engn->jobs.job_queue_lock);

    if(build_send_bd(engn, job))
    {
        PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "failed to build bd and ring door bell");
        goto clean_exit;
    }

    PRINT(DBG_PRAM({IOCTL_DBG}), "ibd sent waiting for it to complete");
    
    if((ret = wait_for_completion_interruptible_timeout(&job->job_completion,JOB_COMP_TIMEOUT)) <= ZERO)
    {
        if(ret < ZERO)
        {
            PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "wait returned negative value restart system");

            for(loop_var = 0;loop_var < total_bd; loop_var++)
            {
                if(bd_ptr[loop_var].done != DONE)
                {
                    del_timer(&bd_ptr[loop_var].bd_timer);
                }
            }
            spin_lock_bh(&engn->jobs.job_queue_lock);
            list_del(&job->list);
            complete_all(&job->job_completion);
            spin_unlock_bh(&engn->jobs.job_queue_lock);
            engn->aborted = 1;
            ret = -ERESTARTSYS;
            err = FAILURE;
        }
        else
        {
            PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "timer expired job with bd id %d and length %d probably stuck", job->bd_tag_start_id, job->total_bd);
            ret = -ETIMEDOUT; 
            err = FAILURE;
        }
        
    }

clean_exit:
    for(loop_var = 0; loop_var < total_bd; loop_var++)
    {
        if(bd_ptr[loop_var].inst_mem == MEM_IOCTL)
        {
            if(bd_ptr[loop_var].bd_type == READ_MEM)
            {
                dma_unmap_sg(&fpga->device_info.pdev->dev,
                             bd_ptr[loop_var].sg_data->sgl,
                             bd_ptr[loop_var].sg_data->nents,
              		         DMA_FROM_DEVICE);
            }
            else
            { 
                dma_map_sg(&fpga->device_info.pdev->dev,
                           bd_ptr[loop_var].sg_data->sgl,
                           bd_ptr[loop_var].sg_data->nents,
              		       DMA_TO_DEVICE);
            }
        }
    }

    if(type == INST)
    {
        unmap_user_memory(fpga->device_info.pdev,
                          inst_dma_addr,
                          direction,
                          inst_malloc,
                          inst_pgs,
                          inst_pg_num,
                          inst_kernel_ptr,
                          inst_ioctl->inst.buff_size);
    }

    unmap_user_pages(sgl_data,
                     dirtied,
                     data_bd,
                     data_malloc,
                     max_bd,
                     data_pgs,
                     data_pg_num,
                     data_kernel_ptr);

    if(type == READ_MEM)
        del_timer(&engn->bar0->interrupt_service.obd_comp_timer);
    else
        del_timer(&engn->bar0->interrupt_service.ibd_comp_timer);

    ZERO_AND_FREE(job->bd);
    ZERO_AND_FREE(job);

    if(err)
        return ret;
    else
        return SUCCESS;
//retry_job:
//    return FAILURE; // space holder change later
    //do not delete job here we will write the logic to redeploy the job for number of times if the thing fails.
}

int ioctl_call(ibmuftian_fpga *fpga, unsigned int ioctl_num, unsigned long args)
{
    int err = ZERO;
    mem_buff mem_ioctl;
    ioctl_inst inst_ioctl;

    if(ioctl_num == WR_EXEC)
    {
        PRINT(DBG_PRAM({IOCTL_DBG}), "instruction ioctl");
        err = copy_from_user(&inst_ioctl, (ioctl_inst __user *)args, sizeof(inst_ioctl));
        if(unlikely(err != 0))
        {
            PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "copy from user failed");
            return err;
        }

        if(unlikely(inst_ioctl.eng_num >= fpga->engns.engn_count))
        {
            PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "engn count not in range");
            return -EADDRNOTAVAIL;
        }
        
        mem_ioctl = inst_ioctl.data;
    }
    else if(ioctl_num == WR_MEM || ioctl_num == RD_MEM)
    {
        PRINT(DBG_PRAM({IOCTL_DBG}), "memory ioctl");
        err = copy_from_user(&mem_ioctl, (mem_buff __user *)args, sizeof(mem_ioctl));
        if(unlikely(err != 0))
        {
            PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "copy from user failed");
            return err;
        }
    }
    else
    {
        PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "invalid ioctl received");
        return -EINVAL;
    }

    if(mem_ioctl.buff_size == 0 || mem_ioctl.buff_size > ((fpga->fpga_info.bit.board_type == FIN) ? TOTAL_FIN_MEM_SIZE : TOTAL_WRASSE_MEM_SIZE))
    {
        PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "memory buffer size greater than the available memory");
        return -EADDRNOTAVAIL;
    }
    
    if(mem_ioctl.uta_ptr > ((fpga->fpga_info.bit.board_type == FIN) ? TOTAL_FIN_MEM_SIZE : TOTAL_WRASSE_MEM_SIZE))
    {
        PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "fpga memory location out of bounds");
        return -EADDRNOTAVAIL;
    }
    
    if(mem_ioctl.uta_ptr + mem_ioctl.buff_size >= ((fpga->fpga_info.bit.board_type == FIN) ? TOTAL_FIN_MEM_SIZE : TOTAL_WRASSE_MEM_SIZE))
    {
        PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "memory offset out of bounds");
        return -EADDRNOTAVAIL;
    }

    switch(ioctl_num)
    {
        case RD_MEM:
        {
            if(!access_ok(VERIFY_WRITE, (void __user *) mem_ioctl.host_ptr, mem_ioctl.buff_size))
            {
                PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "memory pointer not from user space, memory violation may occour");
                return -EADDRNOTAVAIL;
            }
            err = setup_job(fpga, &mem_ioctl, READ_MEM);
            break;
        }
    
        case WR_MEM:
        {
            if(!access_ok(VERIFY_READ, (void __user *) mem_ioctl.host_ptr, mem_ioctl.buff_size))
            {
                PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "memory pointer not from user space, memory violation may occour");
                return -EADDRNOTAVAIL;
            }
            err = setup_job(fpga, &mem_ioctl, WRITE_MEM);
            break;
        }

        case WR_EXEC:
        {
            if(!access_ok(VERIFY_READ, (void __user *) inst_ioctl.inst.host_ptr, inst_ioctl.inst.buff_size))
            {
                PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "memory pointer not from user space, memory violation may occour");
                return -EADDRNOTAVAIL;
            }
            err = setup_job(fpga, &inst_ioctl, INST);
            break;
        }
        default:
        {
            PRINT(DBG_PRAM({IOCTL_DBG, ERR_DBG}), "invalid ioctl received");
            err = -EINVAL;
        }
    }

    return err;
}
