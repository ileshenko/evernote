#include "ibmuftian_interrupt.h"
#include "ibmuftian.h"

u32 get_isr_status(ibmuftian_bar *bar, u32 isr_id, u32 bit)
{
    u32 val = read_bar_addr(bar,bar->register_list[isr_id].status_reg);
    PRINT(DBG_PRAM({INTR_DBG}), "status reg is %8.8X value is %8.8x", bar->register_list[isr_id].status_reg, val&bit);
    return val&bit;
}

void enable_isr(ibmuftian_bar *bar, u32 isr_id, u32 bit)
{
    write_bar_addr(bar,bar->register_list[isr_id].enable_reg, bit);
}

void clear_isr_status(ibmuftian_bar *bar, u32 isr_id, u32 bit)
{
    PRINT(DBG_PRAM({INTR_DBG}), "reg is %8.8X value is %8.8x", bar->register_list[isr_id].status_reg, bit);
    write_bar_addr(bar, bar->register_list[isr_id].status_reg, bit);
}

void ibd_completion_tasklet(unsigned long args)
{
    bd_result * result = NULL;
    ibmuftian_engn * engn = (ibmuftian_engn *)args;
    PRINT(DBG_PRAM({INTR_DBG}), "IBD Completion tasklet");
    if(!engn)
    {
        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "IBD tasklet argument null no engn pointer");
        goto error_exit;
    }

    result = page_address(engn->buff_desc.status_buffs[IBD].status_buff);
    bd_status_ckeck_cleanup(engn, result, IBD);
    
error_exit:
    return;
}

void obd_completion_tasklet(unsigned long args)
{
    bd_result * result = NULL;
    ibmuftian_engn * engn = (ibmuftian_engn *)args;
    
    if(!engn)
    {
        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "OBD tasklet argument null no engn pointer");
        goto error_exit;
    }

    result = page_address(engn->buff_desc.status_buffs[OBD].status_buff);
    bd_status_ckeck_cleanup(engn, result, OBD);

error_exit:
    return;
}

int valid_irq_bitmap(unsigned int data)
{
    return ((data == 0xFFFFFFFF) ? 0:1);
}

void engn_isr(ibmuftian_engn * engn)
{
    ibmuftian_bar * bar = engn->bar0;
    u32 irq_bitmap = read_bar_addr(bar, BAR0_ISR_BITMAP);
    u32 status_reg = 0;
    int irq_loop_var;

    PRINT(DBG_PRAM({INTR_DBG}), "irq bit map is 0x%8.8X", irq_bitmap);
    
    if(!valid_irq_bitmap(irq_bitmap))
    {
        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "irq bitmap not valid returned all 0xfs");
        return;
    }

    for(irq_loop_var = 0; irq_loop_var < MAX_INTERRUPTS; irq_loop_var++)
    {
        if(bits_set(irq_bitmap, 1<<irq_loop_var))
        {
            if(!bar->register_list[irq_loop_var].enable_reg)
            {
                PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "got an interrupt even if its not enabled bit %d set in map reg", irq_loop_var);
            }
            else
            {
                status_reg = get_isr_status(bar, irq_loop_var, ALL_IRQ);

                if(!status_reg)
                {
                    PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "reg %d value is zero", irq_loop_var);
                    continue;
                }
                
                if(irq_loop_var == OBD_COMP_INTR && bits_set(status_reg, OBD_INTR_BIT))
                {
                    PRINT(DBG_PRAM({INTR_DBG}), "received obd completion interrupt");
                    clear_isr_status(bar, irq_loop_var, ALL_IRQ);
                    tasklet_schedule(&bar->interrupt_service.obd_comp_tasklet);
                }
                else if(irq_loop_var == IBD_COMP_INTR &&
                        (bits_set(status_reg, IBD_INTR_MEM_BIT) ||
                          bits_set(status_reg, IBD_INTR_INST_BIT) ||
                          bits_set(status_reg, IBD_INTR_INST_MEM_BIT)))
                {
                    PRINT(DBG_PRAM({INTR_DBG}), "received ibd completion interrupt");
                    clear_isr_status(bar, irq_loop_var, ALL_IRQ);
                    tasklet_schedule(&bar->interrupt_service.ibd_comp_tasklet);
                }
                else if(irq_loop_var == OBD_PARSE_ERR_INTR)
                {
                    if(bits_set(status_reg, OBD_LEN_MORE_1024_INT))
                	{
                	    PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "obd_len_more_1024_int - OBD Length Field is too high");
                    }
                    if(bits_set(status_reg, OBD_INPUT_FIFO_UNDERFLOW_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "obd_input_fifo_underflow_int");
                    }
                    if(bits_set(status_reg, OBD_INPUT_FIFO_OVERFLOW_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "obd_input_fifo_overflow_int");
                    }
                    if(bits_set(status_reg, HMC2HST_ADD_OUT_OF_BNDS_INT_1R))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "OBD HMC(DDR)-2-Host Address-Out-of-Bounds Interrupt Status Register Bit");
                    }
                    if(bits_set(status_reg, HMC2HST_32BT_ADD_BNDRY_INT_1R))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "OBD HMC(DDR)-2-Host 32Byte Address-Boundary Alignment Interrupt Status Register Bit");
                    }
                    if(bits_set(status_reg, OBD_DESCR_BRAM_RD_ERROR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "OBD Buffer Read Error Interrupt_Status_Register Bit");
                    }
                    if(bits_set(status_reg, OBD_DESCR_BRAM_WR_ERROR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "OBD Buffer Write Error Interrupt_Status_Register Bit");
                    }
                } 
                else if(irq_loop_var == IBD_PARSE_ERR_INTR)
                {
                    if(bits_set(status_reg, EP2RC_MRD_4K_FIFO_UNDERFLOW))
                	{
                	      PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "ep2rc_mrd_4k_fifo_overflow - IBD EP2RC_Mrd 4K Fragmentation FIFO is underflowing");
                    }
                    if(bits_set(status_reg, EP2RC_MRD_4K_FIFO_OVERFLOW))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "ep2rc_mrd_4k_fifo_overflow - IBD EP2RC_Mrd 4K Fragmentation FIFO is overflowing");
                    }
                    if(bits_set(status_reg, IBD_LEN_MORE_1024_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "ibd_len_more_1024_int - IBD Length Field is too high");
                    }
                    if(bits_set(status_reg, IBD_INPUT_FIFO_UNDRFL_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "ibd_input_fifo_underflow_int");
                    }
                    if(bits_set(status_reg, IBD_INPUT_FIFO_OVRFL_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Ibd_input_fifo_overflow_int");
                    }
                    if(bits_set(status_reg, HST2HMC_ADD_OUTOF_BNDS_INT_1R))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "IBD Host-2-HMC(DDR) Address-Out-of-Bounds Interrupt Status Register Bit");
                    }
                    if(bits_set(status_reg, HST2HMC_32BT_ADD_NDRY_INT_1R))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "IBD Host-2-HMC(DDR) 32Byte Address-Boundary Alignment Interrupt Status Register Bit");
                    }
                    if(bits_set(status_reg, IBD_DESCR_BRAM_RD_ERROR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "IBD Buffer Read Error Interrupt_Status_Register Bit");
                    }
                    if(bits_set(status_reg, IBD_DESCR_BRAM_WR_ERROR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "IBD Buffer Write Error Interrupt_Status_Register Bit");
                    }
                }
                else if(irq_loop_var == DMA_INGRESS_ERR_INTR)
                {
                    if(bits_set(status_reg, OBD_EP2RC_MRD_FIFO_WR_ERROR))
                	{
                	      PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "obd ep2rc mrd fifo write error");
                    }
                    if(bits_set(status_reg, OBD_EP2RC_MRD_FIFO_RD_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "obd ep2rc mrd fifo read error");
                    }
                    if(bits_set(status_reg, IBD_EP2RC_MRD_FIFO_WR_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "ibd ep2rc mrd fifo write error");
                    }
                    if(bits_set(status_reg, IBD_EP2RC_MRD_FIFO_RD_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "ibd ep2rc mrd fifo read error");
                    }
                    if(bits_set(status_reg, EP2RC_MRD_FIFO_RD_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "ep2rc mrd fifo read error");
                    }
                    if(bits_set(status_reg, EP2RC_MRD_FIFO_WR_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "ep2rc mrd fifo write error");
                    }
                }
                else if(irq_loop_var == DMA_EGRESS_ERR_INTR)
                {
                    if(bits_set(status_reg, OBD_DEST_ADDR_RSP_FIFO_RD_ERR))
                	{
                	    PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "obd destination address response fifo read error");
                    }
                    if(bits_set(status_reg, OBD_DEST_ADDR_RSP_FIFO_WR_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "obd destination address fifo response write error");
                    }
                    if(bits_set(status_reg, OBD_SRC_ADDR_RQ_FIFO_RD_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "obd destination address request fifo read error");
                    }
                    if(bits_set(status_reg, OBD_SRC_ADDR_RQ_FIFO_WR_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "obd source address request fifo write error");
                    }
                    if(bits_set(status_reg, EP2RC_MWR_FIFO_RD_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "EP2RC memory write fifo read error");
                    }
                    if(bits_set(status_reg, EP2RC_MWR_FIFO_WR_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "EP2RC memory write fifo write error");
                    }
                }
                else if(irq_loop_var == DMA_TOP_ERR_INTR)
                {
                    PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "interrupt not implemented should not get this one please check");
                }
                else if(irq_loop_var == MEM_TOP_ERR_INTR)
                {
                    if(bits_set(status_reg, DMA_REQ_FIFO_OFLOW_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "The DMA request fifo was full, but a new DMA request was received");
                    }
                    if(bits_set(status_reg, DMA_RESP_RAM_OFLOW_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "The DMA response RAM received a new response from HMC Bridge, but utaeng_mem_top.v DMA response RAM already contained a pending response for that trans_id");
                    }
                    if(bits_set(status_reg, RESP_RAM_OFLOW_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "uta response reordering ram overflow. May result in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, MEM_RD_RQ_ADDR_ALIGN_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "A UTA read (load) request to memory had nonzero addr[4:0]. Bottom 5 bits are zero'd out when sent to bridge");
                    }
                    if(bits_set(status_reg, MEM_WR_RQ_ADDR_ALIGN_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "A UTA write (store) request to memory had nonzero addr[4:0]   Bottom 5 bits are zero'd out when sent to bridge");
                    }
                    if(bits_set(status_reg, MEM_RD_RQ_SIZE_BNDRY_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "A UTA read (load) request to memory crossed the allowed address boundary-28nm: 16GB-20nm: 8GB Upper bits zero'd out when sent UTA->Bridge)");
                    }
                    if(bits_set(status_reg, MEM_WR_RQ_SIZE_BNDRY_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "A UTA write  (store) request to memory crossed the allowed address boundary-28nm: 16GB-20nm: 8GB Upper bits zero'd out when sent UTA->Bridge)");
                    }
                    if(bits_set(status_reg, MEM_LD_RSP_TIMEOUT_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "waited longer than programmable limit MEM_LOAD_RESP_TIMEOUT to receive load response when at least 1 load was outstanding to bridge.");
                    }
                    if(bits_set(status_reg, MEM_STR_RESP_TIMEOUT_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "waited longer than programmable limit MEM_STORE_RESP_TIMEOUT to receive store ack when at least 1 store was outstanding to bridge.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_MEM_CTL_ERR_IRQ_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from input_fifo's ctl sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_MEM_CTL_WRITE_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from input_fifo's ctl sfifo (overflow. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_MEM_CTL_READ_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from input_fifo's ctl sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_MEM_DATA_ERR_IRQ_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from input_fifo's data [3:0] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_MEM_DATA_WRITE_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from input_fifo's data [3:0] sfifo (overflow). Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_MEM_DATA_READ_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from input_fifo's data [3:0] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, READ_RESP_FIFO_OFLOW_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read response tag fifo (sfifo128) overflow");
                    }
                    if(bits_set(status_reg, READ_RESP_FIFO_UFLOW_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read response tag fifo (sfifo128) underflow");
                    }
                    if(bits_set(status_reg, READ_RESP_FIFO_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read response tag fifo (sfifo128) turned on its int output");
                    }
                }
                else if(irq_loop_var == CSR_TOP_ERR_INTR)
                {
                    PRINT(DBG_PRAM({ERR_DBG}), "Got and interrupt status reg is %8.8X", status_reg);
                    if(bits_set(status_reg, UTAENG_CSR_INGRESS_FIFO_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "fatal error from csr_ingress_fifo");
                    }
                    if(bits_set(status_reg, CSR2PCIE_INVALID_RD_ADDR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "CSR to pcie invalid read address interrupt");
                    }
                    if(bits_set(status_reg, CSR2PCIE_INVALID_WR_ADDR_IN))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "CSR to pcie invalid write address interrupt");
                    }
                }
                else if(irq_loop_var == ENGN_FUNC_REG_ERR_INTR)
                {
                    if(bits_set(status_reg, INVALID_FUNC_TYPE_ENGN_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "New function's func_type field is not recognized");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_CTL_ENGN_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from ctl sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_CTL_WRITE_ENGN_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from ctl sfifo (overflow. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_CTL_READ_ENGN_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from ctl sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_DATA_ENGN_ERR_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from data [0] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_DATA_ENGN_ERR_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from data [1] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_DATA_WRITE_ENGN_ERR_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_DATA_WRITE_ENGN_ERR_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [1] sfifo (overflow). Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_DATA_READ_ENGN_ERR_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [0] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_DATA_READ_ENGN_ERR_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [1] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                }
                else if(irq_loop_var == ENGN_TOP_REG_ERR_INTR)
                {
                    if(bits_set(status_reg, MMAP_RD_ADDR_ENGNR_RSVD_ERR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Received a memory mapped op that doesn't fall within any reg's valid address range");
                    }
                    if(bits_set(status_reg, MMAP_WR_ADDR_ENGNR_RSVD_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Received a memory mapped op that doesn't fall within any reg's valid address range");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNR_CTL_ERR_IRQ_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from ctl sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_CTL_ENGN_WRITE_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from ctl sfifo (overflow. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_CTL_ENGN_READ_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from ctl sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGN_DATA_ERR_IRQ_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from data [0] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGN_DATA_ERR_IRQ_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from data [1] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGN_DATA_WRITE_ERR_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGN_DATA_WRITE_ERR_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [1] sfifo (overflow). Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGN_DATA_READ_ERR_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [0] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGN_DATA_READ_ERR_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [1] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, ADDR_COUNT_WR_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "address collision logic fifo encountered an overflow. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, ADDR_COUNT_RD_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "address collision logic fifo encountered an underflow. Results in hang or corrupted data.");
                    }
                }
                else if(irq_loop_var == ENGN_FUNC_ERR_INTR)
                {
                    PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "interrupt not implemented should not get this one please check");
                }
                else if(irq_loop_var == ENGN_FUNC_VSRF_ERR_INTR)
                {
                    if(bits_set(status_reg, INVALID_FUNC_TYPE_VSRF_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "New function's func_type field is not recognized");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNF_CTL_ERR_IRQ_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from ctl sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNF_CTL_WRITE_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from ctl sfifo (overflow. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNF_CTL_READ_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from ctl sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNF_DATA_ERR_IRQ_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from data [0] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNF_DATA_ERR_IRQ_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from data [1] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNF_DATA_WRITE_ERR_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNF_DATA_WRITE_ERR_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [1] sfifo (overflow). Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNF_DATA_READ_ERR_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [0] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNF_DATA_READ_ERR_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [1] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                }
                else if(irq_loop_var == ENGN_VSRF_TOP_ERR_INTR)
                {
                    if(bits_set(status_reg, INPUT_FIFO_ENGNV_CTL_ERR_IRQ_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from ctl sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNV_CTL_WRITE_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from ctl sfifo (overflow. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNV_CTL_READ_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from ctl sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNV_DATA_ERR_IRQ_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from data [7:0] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNV_DATA_WRITE_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [7:0] sfifo (overflow). Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNV_DATA_READ_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [7:0] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, VPUTD_BAD_DEST_ADDR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "vector put dword bad destination address");
                    }
                    if(bits_set(status_reg, VGETPW_ADDR_NOT_EVEN))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "vector get address not even");
                    }
                    if(bits_set(status_reg, VENQ_LEN_MORE_32_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "vector enqueue length more than 32");
                    }
                    if(bits_set(status_reg, GATH_VSRF_NON64B_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG}), "gather vsrf not 64 bytes");  //this is being triggered in all the tests so not setting it as error.
                    }
                }
                else if(irq_loop_var == ENGN_FUNC_MR_ERR_INTR)
                {
                    if(bits_set(status_reg, INVALID_FUNC_TYPE_MR_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "New function's func_type field is not recognized");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNFM_CTL_ERR_IRQ_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from ctl sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNFM_CTL_WRITE_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from ctl sfifo (overflow. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNFM_CTL_READ_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from ctl sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNFM_DATA_ERR_IRQ_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from data [0] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNFM_DATA_ERR_IRQ_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from data [1] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNFM_DATA_WRITE_ERR_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNFM_DATA_WRITE_ERR_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [1] sfifo (overflow). Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNFM_DATA_READ_ERR_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [0] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNFM_DATA_READ_ERR_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [1] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                }
                else if(irq_loop_var == ENGN_MAP_REG_TOP_ERR_INTR)
                {
                    if(bits_set(status_reg, INVALID_ADDR_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "This condition is not covered by the address collision logic for multi-beat ops, so is not allowed. Could result in a hang or corrupted VSRF data");
                    }
                    if(bits_set(status_reg, ADDR_COUNT_VSRF_RD_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "vsrf addr_count fifo underflow");
                    }
                    if(bits_set(status_reg, ADDR_COUNT_VSRF_WR_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "vsrf addr_count fifo overflow");
                    }
                    if(bits_set(status_reg, ADDR_COUNT_MR_RD_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "mapreg addr_count fifo underflow");
                    }
                    if(bits_set(status_reg, ADDR_COUNT_MR_WR_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "mapreg addr_count fifo overflow");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNMR_DATA_READ_ERR_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [1] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNMR_DATA_READ_ERR_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from data [0] sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNMR_DATA_WRITE_ERR_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from data [1] sfifo (overflow). Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNMR_DATA_WRITE_ERR_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "  write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNMR_DATA_ERR_IRQ_Q_1))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "  err_irq error from data [1] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNMR_DATA_ERR_IRQ_Q_0))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "  err_irq error from data [0] sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNMR_CTL_READ_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "read err from ctl sfifo (underflow) . Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNMR_CTL_WRITE_ERR_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "write_err from ctl sfifo (overflow. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, INPUT_FIFO_ENGNMR_CTL_ERR_IRQ_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "err_irq error from ctl sfifo. Results in hang or corrupted data.");
                    }
                    if(bits_set(status_reg, MMAP_WR_ADDR_ENGNMR_RSVD_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Received a memory mapped op that doesn't fall within the mapreg's valid address range");
                    }
                    if(bits_set(status_reg, MMAP_RD_ADDR_ENGNMR_RSVD_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Received a memory mapped op that doesn't fall within the mapreg's valid address range");
                    }
                    if(bits_set(status_reg, COLLISION_MMAP_UTA_WR_ERR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Received a memory mapped and uta write in the same cycle");
                    }
                    if(bits_set(status_reg, COLLISION_MMAP_UTA_RD_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Received a memory mapped and uta read in the same cycle");
                    }
                }
                else if(irq_loop_var == ENGN_DISPATCH_ERR_INTR)
                {
                    if(bits_set(status_reg, FIFO_RD_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Dispatcher fifo underflow error. Opcodes from this point on not reliable.");
                    }
                    if(bits_set(status_reg, FIFO_WR_ERR_INT_Q))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Dispatcher fifo overflow error. Opcodes from this point on not reliable.");
                    }
                }
                else if(irq_loop_var == ENG_LKUP_PKTZR_ERR_INTR)
                {
                    if(bits_set(status_reg, BEAT_LEN_FIFO_DBIT_ERR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "beat length fifo has an uncorrectable double-bit ECC error.");
                    }
                    if(bits_set(status_reg, INSTQ_FIFO_RD_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "instruction fifo read error");
                    }
                    if(bits_set(status_reg, INSTQ_FIFO_WR_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "instruction fifo write error");
                    }
                    if(bits_set(status_reg, BEAT_LEN_RD_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "beat lengthe read error");
                    }
                    if(bits_set(status_reg, BEAT_LEN_WR_ERR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "beat length write error");
                    }
                    if(bits_set(status_reg, MMAP2LU_LEN_RD_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "memory map to LU length read error");
                    }
                    if(bits_set(status_reg, MMAP2LU_LEN_WR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "memory map to LU length write error");
                    }
                }
                
                if(irq_loop_var == ENGN_INST_QUEUE_ERR_INTR)
                {
                    if(bits_set(status_reg, INSTRQ_BRAM_RD_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "instruction queue b ram read error");
                    }
                    if(bits_set(status_reg, INSTRQ_BRAM_WR_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "instruction queue b ram write error");
                    }
                    if(bits_set(status_reg, EP2RC_CPLD_FIFO_WR_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "EP2RC CPLD fifo write error");
                    }
                    if(bits_set(status_reg, EP2RC_CPLD_FIFO_RD_ERROR))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "EP2RC CPLD fifo read error");
                    }
                    if(bits_set(status_reg, INSTRQ_TST_RD_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "instruction queue test read intr");
                    }
                    if(bits_set(status_reg, INSTRQ_TST_WR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "instruction queue test write intr");
                    }
                    if(bits_set(status_reg, INSTRQ_OFLOW_ERR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "Fatal condition system down");
                    }
                }

                clear_isr_status(bar, irq_loop_var, ALL_IRQ);
                status_reg = get_isr_status(bar,irq_loop_var,ALL_IRQ);
                PRINT(DBG_PRAM({INTR_DBG}), "status returned with %8.8X",status_reg);
            }
        }
    }
}

irqreturn_t ibmuftian_isr(int irq, void *args)
{
    int id = 0;
    u32 irq_bitmap = 0, status_reg = 0;
    ibmuftian_bar * bar;
    ibmuftian_fpga * fpga = (ibmuftian_fpga *)args;

    PRINT(DBG_PRAM({INTR_DBG}), "interrupt occoured on fpga %d", fpga->id);
        
    if(unlikely(!fpga))
    {
        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "irq argument null no fpga pointer");
        return IRQ_NONE;
    }

    bar = fpga->bar.bar4;

    if(unlikely(!bar))
    {
        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "fpga bar4 pointer null");
        goto exit;
    }

    while(1)
    {
        if(!(irq_bitmap = read_bar_addr(bar,BAR4_ISR_BITMAP)))
        {
            PRINT(DBG_PRAM({INTR_DBG}), "irq register 0 no irq received");
            break;
        }

        PRINT(DBG_PRAM({INTR_DBG}), "Received  bar45 bitmap 0x%8.8X \n",irq_bitmap);

        if(!valid_irq_bitmap(irq_bitmap))
        {
            PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "irq bitmap not valid returned all 0xfs");
            break;
        }

        for(id = 0; id < fpga->engns.engn_count; id++)
        {
            if(bits_set(irq_bitmap, 1<<id))
            {
                PRINT(DBG_PRAM({INTR_DBG}), "engn %d interrupt occoured", id); 
                engn_isr(fpga->engns.engines[id]);
            }
        }
        for(id = 24; id < 28; id++)
        {
            if(bits_set(irq_bitmap, 1<<id))
            {
                status_reg = get_isr_status(bar, id, ALL_IRQ);
                if(id == HMC3_CONTROL_INT)
                {
                    if(bits_set(status_reg,HMC_CORE_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "HMC 3 core interrupt");
                    }
                    else if(bits_set(status_reg,HMC_FERR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "HMC 3 fatal error interrupt");
                    }
                }
                else if(id == HMC2_CONTROL_INT)
                {
                    if(bits_set(status_reg,HMC_CORE_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "HMC 2 core interrupt");
                    }
                    else if(bits_set(status_reg,HMC_FERR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "HMC 2 fatal error interrupt");
                    }
                }
                else if(id == HMC1_CONTROL_INT)
                {
                    if(bits_set(status_reg,HMC_CORE_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "HMC 1 core interrupt");
                    }
                    else if(bits_set(status_reg,HMC_FERR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "HMC 1 fatal error interrupt");
                    }
                }
                else if(id == HMC0_CONTROL_INT)
                {
                    if(bits_set(status_reg,HMC_CORE_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "HMC 0 core interrupt");
                    }
                    else if(bits_set(status_reg,HMC_FERR_INT))
                    {
                        PRINT(DBG_PRAM({INTR_DBG, ERR_DBG}), "HMC 0 fatal error interrupt");
                    }
                }
            }
        }
    }

    ACK_INTERRUPT(bar);
exit:
    return IRQ_HANDLED;
}

