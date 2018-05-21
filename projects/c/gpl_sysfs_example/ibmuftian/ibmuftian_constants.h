#ifndef _IBMUFTIAN_CONSTANTS_H_
#define _IBMUFTIAN_CONSTANTS_H_

/****************************General constants*************************************/

#define IBMUFTIAN_NAME                            "ibmuftian"
#define IBMUFTIAN_DESC                            "ibm universal accelerator card driver"
#define BOARD_NAME_FIN                            "fin"
#define BOARD_NAME_WRASSE                         "wrasse"
#define IBMUFTIAN_VERSION                         "1.0"
#define BUFF_LEN                                  256
#define IBMUFTIAN_PCI_VENDOR_ID                   0x1014
#define IBMUFTIAN_PCI_FIN_DEVICE_ID               0x04C5    
#define FIN                                       1
#define WRASSE                                    2

#define ZERO                                      0

#define SUCCESS                                   0
#define FAILURE                                   -1

#define NAME_LEN                                  20
#define NUMBER_OF_ENGNS                           6
#define RESERVED_ENGN_INFO_LEN                    2
#define BOARD_TYPE_LEN                            4
#define REVISION_LEN                              8
#define VERSION_LEN                               4   
#define DATE_LEN                                  24
#define PRODUCT_LEN                               8  
#define BOARD_NAME_LEN                            BUFF_LEN 
#define DMA_BIT_MASK_LEN                          64
#define MLN_NO                                    4

#define DISABLE                                   0
#define ENABLE                                    1

#define ENABLE_ALL                                0xFFFFFFFF
#define DISABLE_ALL                               0x00000000

#define TOTAL_FIN_MEM_SIZE                        0x400000000
#define TOTAL_WRASSE_MEM_SIZE                     0x200000000
#define MEM_KEY                                   "yayY!c4=E^-fa#k^n^d&@&%Q*s8WQ#v6SYWm!bE_yVsUrzVTW%L2aY8qR=?vN?Ws"

#define SPACE                                     10, ""
#define HEX                                       16
#define DEC                                       10

/***********************BOARD related constants***************************************/

// This is how much data we actually define (including unused fields) in the chip.
#define VPD_SIZE                                  (4 * 1024)
#define VPD_OFFSET                                0 /* Offset within the EEPROM device, used for I2C */

// These are lengths, there are no null termination chars on VPD strings. These
// lengths are defined by the IBM VPD specification and cannot change.
#define CARD_SERIAL_NUMBER_LEN                    6
#define CARD_PREFIX_SERIAL_NUMBER_LEN             6
#define HARDWARE_REVISION_LEVEL_LEN               1
#define IBM_PRODUCT_NAME_LEN                      64

// Offsets from beginning of either the VPD EEPROM or the VPD buffer.
#define VPD_OFS                                   0x0000
#define CARD_SERIAL_NUMBER_OFS                    0x0054
#define CARD_PREFIX_SERIAL_NUMBER_OFS             0x005A
#define HARDWARE_REVISION_LEVEL_OFS               0x0065
#define IBM_PRODUCT_NAME_OFS                      0x010E

// Sizes include the nul char.
#define VPD_SERNO_SIZE                            (CARD_SERIAL_NUMBER_LEN + CARD_PREFIX_SERIAL_NUMBER_LEN + 1)
#define VPD_HWREV_SIZE                            (2 + 2 + 1) /* '0x' + 2 hex digits per byte + nul char */
#define VPD_DESC_SIZE                             (IBM_PRODUCT_NAME_LEN + 1)

#define I2C_COMMAND_MAX_LENGTH                    0x1000 //4096
#define I2C_COMMAND_MAX_INSTR                     10
#define I2C_COMMAND_MAX_INSTR_NAME                10
#define I2C_COMMAND_MAX_INSTR_PARAM               32

/****************************FPGA related constants********************************************/

#define FPGA_INFO_ADDR                            0x00000004
#define FPGA_ENGN_INFO_ADDR                       0x00000000

/****************************INTERRUPT related constants********************************************/

#define GENERAL_MSI_ACK                           0x6880
#define ACK_VAL                                   0x211

#define ALL_IRQ                                   0xFFFFFFFF
#define BAR4_ISR_BITMAP                           0x00006000
#define BAR0_ISR_BITMAP                           0x00046000
#define MAX_INTERRUPTS                            32

#define OBD_COMP_INTR                             31 
#define OBD_INTR_BIT                              1<<0

#define IBD_COMP_INTR                             30
#define IBD_INTR_MEM_BIT                          1<<0
#define IBD_INTR_INST_BIT                         1<<1
#define IBD_INTR_INST_MEM_BIT                     1<<2

#define OBD_PARSE_ERR_INTR                        29
#define OBD_LEN_MORE_1024_INT	                  1<<6 //obd_len_more_1024_int - OBD Length Field is too high
#define OBD_INPUT_FIFO_UNDERFLOW_INT	          1<<5 //obd_input_fifo_underflow_int
#define OBD_INPUT_FIFO_OVERFLOW_INT	              1<<4 //obd_input_fifo_overflow_int
#define HMC2HST_ADD_OUT_OF_BNDS_INT_1R	          1<<3 //OBD HMC(DDR)-2-Host Address-Out-of-Bounds Interrupt Status Register Bit
#define HMC2HST_32BT_ADD_BNDRY_INT_1R	          1<<2 //OBD HMC(DDR)-2-Host 32Byte Address-Boundary Alignment Interrupt Status Register Bit
#define OBD_DESCR_BRAM_RD_ERROR_INT               1<<1 //OBD Buffer Read Error Interrupt_Status_Register Bit
#define OBD_DESCR_BRAM_WR_ERROR_INT	              1<<0 //OBD Buffer Write Error Interrupt_Status_Register Bit

#define IBD_PARSE_ERR_INTR                        28
#define EP2RC_MRD_4K_FIFO_UNDERFLOW	              1<<8 //ep2rc_mrd_4k_fifo_overflow - IBD EP2RC_Mrd 4K Fragmentation FIFO is underflowing
#define EP2RC_MRD_4K_FIFO_OVERFLOW	              1<<7 //ep2rc_mrd_4k_fifo_overflow - IBD EP2RC_Mrd 4K Fragmentation FIFO is overflowing
#define IBD_LEN_MORE_1024_INT	                  1<<6 //ibd_len_more_1024_int - IBD Length Field is too high
#define IBD_INPUT_FIFO_UNDRFL_INT	              1<<5 //ibd_input_fifo_underflow_int
#define IBD_INPUT_FIFO_OVRFL_INT	              1<<4 //Ibd_input_fifo_overflow_int
#define HST2HMC_ADD_OUTOF_BNDS_INT_1R	          1<<3 //IBD Host-2-HMC(DDR) Address-Out-of-Bounds Interrupt Status Register Bit
#define HST2HMC_32BT_ADD_NDRY_INT_1R	          1<<2 //IBD Host-2-HMC(DDR) 32Byte Address-Boundary Alignment Interrupt Status Register Bit
#define IBD_DESCR_BRAM_RD_ERROR_INT	              1<<1 //IBD Buffer Read Error Interrupt_Status_Register Bit
#define IBD_DESCR_BRAM_WR_ERROR_INT	              1<<0 //IBD Buffer Write Error Interrupt_Status_Register Bit

#define DMA_INGRESS_ERR_INTR                      26
#define OBD_EP2RC_MRD_FIFO_WR_ERROR               1<<5
#define OBD_EP2RC_MRD_FIFO_RD_ERROR               1<<4
#define IBD_EP2RC_MRD_FIFO_WR_ERROR               1<<3
#define IBD_EP2RC_MRD_FIFO_RD_ERROR               1<<2
#define EP2RC_MRD_FIFO_RD_ERROR                   1<<1
#define EP2RC_MRD_FIFO_WR_ERROR                   1<<0

#define DMA_EGRESS_ERR_INTR                       25
#define OBD_DEST_ADDR_RSP_FIFO_RD_ERR             1<<5
#define OBD_DEST_ADDR_RSP_FIFO_WR_ERR             1<<4
#define OBD_SRC_ADDR_RQ_FIFO_RD_ERR               1<<3
#define OBD_SRC_ADDR_RQ_FIFO_WR_ERR               1<<2
#define EP2RC_MWR_FIFO_RD_ERROR                   1<<1
#define EP2RC_MWR_FIFO_WR_ERROR                   1<<0

#define DMA_TOP_ERR_INTR                          24

#define MEM_TOP_ERR_INTR                          22
#define DMA_REQ_FIFO_OFLOW_ERR_INT_Q 	          1<<31 //The DMA request fifo was full, but a new DMA request was received
#define DMA_RESP_RAM_OFLOW_ERR_INT_Q 	          1<<30 //The DMA response RAM received a new response from HMC Bridge, but utaeng_mem_top.v DMA response RAM already contained a pending response for that trans_id
#define RESP_RAM_OFLOW_ERR_INT_Q	              1<<28 //uta response reordering ram overflow. May result in hang or corrupted data.
#define MEM_RD_RQ_ADDR_ALIGN_ERR_INT_Q 	          1<<27 //A UTA read (load) request to memory had nonzero addr[4:0]. Bottom 5 bits are zero'd out when sent to bridge
#define MEM_WR_RQ_ADDR_ALIGN_ERR_INT_Q 	          1<<26 //A UTA write (store) request to memory had nonzero addr[4:0]   Bottom 5 bits are zero'd out when sent to bridge
#define MEM_RD_RQ_SIZE_BNDRY_ERR_INT_Q 	          1<<25 //A UTA read (load) request to memory crossed the allowed address boundary-28nm: 16GB-20nm: 8GB Upper bits zero'd out when sent UTA->Bridge) 
#define MEM_WR_RQ_SIZE_BNDRY_ERR_INT_Q 	          1<<24 //A UTA write  (store) request to memory crossed the allowed address boundary-28nm: 16GB-20nm: 8GB Upper bits zero'd out when sent UTA->Bridge)
#define MEM_LD_RSP_TIMEOUT_ERR_INT_Q	          1<<23 //waited longer than programmable limit MEM_LOAD_RESP_TIMEOUT to receive load response when at least 1 load was outstanding to bridge.
#define MEM_STR_RESP_TIMEOUT_ERR_INT_Q	          1<<22 //waited longer than programmable limit MEM_STORE_RESP_TIMEOUT to receive store ack when at least 1 store was outstanding to bridge.
#define INPUT_FIFO_MEM_CTL_ERR_IRQ_Q	          1<<19 //err_irq error from input_fifo's ctl sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_MEM_CTL_WRITE_ERR_Q	          1<<18 //write_err from input_fifo's ctl sfifo (overflow. Results in hang or corrupted data.
#define INPUT_FIFO_MEM_CTL_READ_ERR_Q	          1<<17 //read err from input_fifo's ctl sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_MEM_DATA_ERR_IRQ_Q   	      0b1111<<13 //err_irq error from input_fifo's data [3:0] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_MEM_DATA_WRITE_ERR_Q	          0b1111<<9 //write_err from input_fifo's data [3:0] sfifo (overflow). Results in hang or corrupted data.
#define INPUT_FIFO_MEM_DATA_READ_ERR_Q	          0b1111<<5 //read err from input_fifo's data [3:0] sfifo (underflow) . Results in hang or corrupted data.
#define READ_RESP_FIFO_OFLOW_ERR_INT_Q	          1<<4 //read response tag fifo (sfifo128) overflow
#define READ_RESP_FIFO_UFLOW_ERR_INT_Q	          1<<3 //read response tag fifo (sfifo128) underflow
#define READ_RESP_FIFO_ERR_INT_Q	              1<<2 //read response tag fifo (sfifo128) turned on its "int" output

#define CSR_TOP_ERR_INTR                          20
#define UTAENG_CSR_INGRESS_FIFO_INT	              1<<2 //fatal error from csr_ingress_fifo
#define CSR2PCIE_INVALID_RD_ADDR_INT              1<<1
#define CSR2PCIE_INVALID_WR_ADDR_IN               1<<0

#define ENGN_FUNC_REG_ERR_INTR                    18
#define INVALID_FUNC_TYPE_ENGN_ERR	              1<<31 //New function's func_type field is not recognized
#define INPUT_FIFO_CTL_ENGN_ERR  	              1<<30 //err_irq error from ctl sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_CTL_WRITE_ENGN_ERR	          1<<29 //write_err from ctl sfifo (overflow. Results in hang or corrupted data.
#define INPUT_FIFO_CTL_READ_ENGN_ERR	          1<<28 //read err from ctl sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_DATA_ENGN_ERR_0	              1<<27 //err_irq error from data [0] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_DATA_ENGN_ERR_1	              1<<26 //err_irq error from data [1] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_DATA_WRITE_ENGN_ERR_0          1<<25 //write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.
#define INPUT_FIFO_DATA_WRITE_ENGN_ERR_1          1<<24 //write_err from data [1] sfifo (overflow). Results in hang or corrupted data.
#define INPUT_FIFO_DATA_READ_ENGN_ERR_0	          1<<23 //read err from data [0] sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_DATA_READ_ENGN_ERR_1	          1<<22 //read err from data [1] sfifo (underflow) . Results in hang or corrupted data.

#define ENGN_TOP_REG_ERR_INTR                     16
#define MMAP_RD_ADDR_ENGNR_RSVD_ERR_INT	          1<<30 //Received a memory mapped op that doesn't fall within any reg's valid address range
#define MMAP_WR_ADDR_ENGNR_RSVD_ERR_INT_Q	      1<<29 //Received a memory mapped op that doesn't fall within any reg's valid address range
#define INPUT_FIFO_ENGNR_CTL_ERR_IRQ_Q	          1<<28 //err_irq error from ctl sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_CTL_ENGN_WRITE_ERR_Q	          1<<27 //write_err from ctl sfifo (overflow. Results in hang or corrupted data.
#define INPUT_FIFO_CTL_ENGN_READ_ERR_Q	          1<<26 //read err from ctl sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGN_DATA_ERR_IRQ_Q_0	      1<<25 //err_irq error from data [0] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGN_DATA_ERR_IRQ_Q_1	      1<<24 //err_irq error from data [1] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGN_DATA_WRITE_ERR_Q_0	      1<<23 //write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGN_DATA_WRITE_ERR_Q_1	      1<<22 //write_err from data [1] sfifo (overflow). Results in hang or corrupted data.
#define INPUT_FIFO_ENGN_DATA_READ_ERR_Q_0	      1<<21 //read err from data [0] sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGN_DATA_READ_ERR_Q_1	      1<<20 //read err from data [1] sfifo (underflow) . Results in hang or corrupted data.
#define ADDR_COUNT_WR_ERR_INT_Q	                  1<<19 //address collision logic fifo encountered an overflow. Results in hang or corrupted data.
#define ADDR_COUNT_RD_ERR_INT_Q	                  1<<18 //address collision logic fifo encountered an underflow. Results in hang or corrupted data.

#define ENGN_FUNC_ERR_INTR                        14

#define ENGN_FUNC_VSRF_ERR_INTR                   12
#define INVALID_FUNC_TYPE_VSRF_ERR_INT_Q	      1<<31 //New function's func_type field is not recognized
#define INPUT_FIFO_ENGNF_CTL_ERR_IRQ_Q	          1<<30 //err_irq error from ctl sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNF_CTL_WRITE_ERR_Q	      1<<29 //write_err from ctl sfifo (overflow. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNF_CTL_READ_ERR_Q	          1<<28 //read err from ctl sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNF_DATA_ERR_IRQ_Q_0	      1<<27 //err_irq error from data [0] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNF_DATA_ERR_IRQ_Q_1	      1<<26 //err_irq error from data [1] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNF_DATA_WRITE_ERR_Q_0	      1<<25 //write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNF_DATA_WRITE_ERR_Q_1	      1<<24 //write_err from data [1] sfifo (overflow). Results in hang or corrupted data.
#define INPUT_FIFO_ENGNF_DATA_READ_ERR_Q_0	      1<<23 //read err from data [0] sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNF_DATA_READ_ERR_Q_1	      1<<22 //read err from data [1] sfifo (underflow) . Results in hang or corrupted data.

#define ENGN_VSRF_TOP_ERR_INTR                    10
#define INPUT_FIFO_ENGNV_CTL_ERR_IRQ_Q	          1<<31 //err_irq error from ctl sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNV_CTL_WRITE_ERR_Q	      1<<30 //write_err from ctl sfifo (overflow. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNV_CTL_READ_ERR_Q	          1<<29 //read err from ctl sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNV_DATA_ERR_IRQ_Q           0b11111111<<21 //err_irq error from data [7:0] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNV_DATA_WRITE_ERR_Q         0b11111111<<13 //write_err from data [7:0] sfifo (overflow). Results in hang or corrupted data.
#define INPUT_FIFO_ENGNV_DATA_READ_ERR_Q   	      0b11111111<<5 //read err from data [7:0] sfifo (underflow) . Results in hang or corrupted data.
#define VPUTD_BAD_DEST_ADDR_INT	                  1<<4
#define VGETPW_ADDR_NOT_EVEN	                  1<<2
#define VENQ_LEN_MORE_32_INT	                  1<<1
#define GATH_VSRF_NON64B_INT	                  1<<0

#define ENGN_FUNC_MR_ERR_INTR                     8
#define INVALID_FUNC_TYPE_MR_ERR_INT_Q	          1<<31 //New function's func_type field is not recognized
#define INPUT_FIFO_ENGNFM_CTL_ERR_IRQ_Q	          1<<29 //err_irq error from ctl sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNFM_CTL_WRITE_ERR_Q	      1<<28 //write_err from ctl sfifo (overflow. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNFM_CTL_READ_ERR_Q	      1<<27 //read err from ctl sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNFM_DATA_ERR_IRQ_Q_0	      1<<26 //err_irq error from data [0] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNFM_DATA_ERR_IRQ_Q_1	      1<<25 //err_irq error from data [1] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNFM_DATA_WRITE_ERR_Q_0	  1<<24 //write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNFM_DATA_WRITE_ERR_Q_1	  1<<23 //write_err from data [1] sfifo (overflow). Results in hang or corrupted data.
#define INPUT_FIFO_ENGNFM_DATA_READ_ERR_Q_0	      1<<22 //read err from data [0] sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNFM_DATA_READ_ERR_Q_1	      1<<21 //read err from data [1] sfifo (underflow) . Results in hang or corrupted data.

#define ENGN_MAP_REG_TOP_ERR_INTR                 6
#define INVALID_ADDR_ERR_INT_Q                    1<<12 //This condition is not covered by the address collision logic for multi-beat ops, so is not allowed. Could result in a hang or corrupted VSRF data
#define ADDR_COUNT_VSRF_RD_ERR_INT_Q              1<<13 //vsrf addr_count fifo underflow
#define ADDR_COUNT_VSRF_WR_ERR_INT_Q              1<<14 //vsrf addr_count fifo overflow
#define ADDR_COUNT_MR_RD_ERR_INT_Q                1<<15 //mapreg addr_count fifo underflow
#define ADDR_COUNT_MR_WR_ERR_INT_Q                1<<16 //mapreg addr_count fifo overflow
#define INPUT_FIFO_ENGNMR_DATA_READ_ERR_Q_1       1<<17 //read err from data [1] sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNMR_DATA_READ_ERR_Q_0       1<<18 //read err from data [0] sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNMR_DATA_WRITE_ERR_Q_1      1<<19 //write_err from data [1] sfifo (overflow). Results in hang or corrupted data.
#define INPUT_FIFO_ENGNMR_DATA_WRITE_ERR_Q_0      1<<20 //	write_err from data [0] sfifo (overflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNMR_DATA_ERR_IRQ_Q_1        1<<21 //	err_irq error from data [1] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNMR_DATA_ERR_IRQ_Q_0        1<<22 //	err_irq error from data [0] sfifo. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNMR_CTL_READ_ERR_Q	      1<<23 //read err from ctl sfifo (underflow) . Results in hang or corrupted data.
#define INPUT_FIFO_ENGNMR_CTL_WRITE_ERR_Q	      1<<24 //write_err from ctl sfifo (overflow. Results in hang or corrupted data.
#define INPUT_FIFO_ENGNMR_CTL_ERR_IRQ_Q	          1<<25 //err_irq error from ctl sfifo. Results in hang or corrupted data.
#define MMAP_WR_ADDR_ENGNMR_RSVD_ERR_INT_Q        1<<27 //Received a memory mapped op that doesn't fall within the mapreg's valid address range 
#define MMAP_RD_ADDR_ENGNMR_RSVD_ERR_INT_Q        1<<28 //Received a memory mapped op that doesn't fall within the mapreg's valid address range 
#define COLLISION_MMAP_UTA_WR_ERR_INT             1<<29 //Received a memory mapped and uta write in the same cycle
#define COLLISION_MMAP_UTA_RD_ERR_INT_Q           1<<30 //Received a memory mapped and uta read in the same cycle 

#define ENGN_DISPATCH_ERR_INTR                    4
#define FIFO_RD_ERR_INT_Q                         1<<30 //Dispatcher fifo underflow error. Opcodes from this point on not reliable.
#define FIFO_WR_ERR_INT_Q                         1<<31 //Dispatcher fifo overflow error. Opcodes from this point on not reliable.

#define ENG_LKUP_PKTZR_ERR_INTR                   2
#define BEAT_LEN_FIFO_DBIT_ERR_INT                1<<2 //beat length fifo has an uncorrectable double-bit ECC error.
#define INSTQ_FIFO_RD_ERR                         1<<3
#define INSTQ_FIFO_WR_ERR                         1<<4
#define BEAT_LEN_RD_ERR                           1<<5
#define BEAT_LEN_WR_ERR                           1<<6
#define MMAP2LU_LEN_RD_INT                        1<<7  
#define MMAP2LU_LEN_WR_INT                        1<<8

#define ENGN_INST_QUEUE_ERR_INTR                  0
#define INSTRQ_BRAM_RD_ERROR                      1<<2
#define INSTRQ_BRAM_WR_ERROR                      1<<3
#define EP2RC_CPLD_FIFO_WR_ERROR                  1<<4
#define EP2RC_CPLD_FIFO_RD_ERROR                  1<<5
#define INSTRQ_TST_RD_INT                         1<<6 
#define INSTRQ_TST_WR_INT                         1<<7
#define INSTRQ_OFLOW_ERR_INT                      1<<7 //Fatal condition system down

#define HMC3_CONTROL_INT                           27
#define HMC2_CONTROL_INT                           26
#define HMC1_CONTROL_INT                           25
#define HMC0_CONTROL_INT                           24
#define HMC_FERR_INT                               1<<16
#define HMC_CORE_INT                               1<<0

/****************************ENGN related constants********************************************/

#define IBD                                       0
#define OBD                                       1
#define STATUS_BUFF_LEN_DWORD                     32
#define NUM_STATUS_BUFF                           32

#define LAST_JOB                                  1

#define DEADBEEF                                  0xdeadbeef
#define FEEDFACE                                  0xfeedface

#define STATUS_IBD_LEN                            0x00044000
#define STATUS_IBD_ADDR_HI                        0x00044004
#define STATUS_IBD_ADDR_LO                        0x00044008

#define STATUS_OBD_LEN                            0x00044014
#define STATUS_OBD_ADDR_HI                        0x00044018
#define STATUS_OBD_ADDR_LO                        0x0004401C

/****************************IOCTL related constants********************************************/

#define READ_MEM                                  0
#define WRITE_MEM                                 1
#define INST                                      2

#define INST_IOCTL                                1
#define MEM_IOCTL                                 0

#define DIRTY                                     1

#define MAX_DATA_SEGS                             510
#define DMA_DATA_SIZE                             0x1000

#define INTERRUPT_ENABLE                          8
#define BD_IN_INST_IOCTL                          2
#define LAST_MEM_BD                               2
#define INST_BD_OFFSET                            1
#define BD_TYPES                                  2
#define JOB_COMP_TIMEOUT                          100*HZ

#define IN_PROGRESS                               0
#define DONE                                      1

#define MAX_BD_SIZE                               4096
#define BD_FIFO_LEN                               32

#define INST_BD_SIZE                              276
#define BASE_MEM_BD_LEN                           16

#define SET_INTERRUPT                             1

#define DWORD_LEN                                 4
#define ADDR_LEN                                  8

#define MEM_BD_TIMEOUT                            (35*HZ)
#define INST_BD_TIMEOUT                           (70*HZ)
#define BD_MAX_GPR_REGS                           32
#define BD_MAX_HOST_PTR_REGS                      510
#define DESC_ID_LEN                               16
#define LEN_BITS                                  10
#define INTR_BIT_LEN                              1
#define TYPE_BIT_LEN                              1
#define REDO                                      1

#define RESERVED_DWORDS_IN_STATUS                 26

#define GPR_LOW                                   8
#define GPR_HIGH                                  8+4                //keep it this way

#define IBD_LEN_ADDR	                          0x00044020
#define IBD_ADDR_HI                               0x00044024
#define IBD_ADDR_LO                               0x00044028

#define OBD_LEN_ADDR                              0x00044080
#define OBD_ADDR_HI                               0x00044084
#define OBD_ADDR_LO                               0x00044088
/****************************PCI BAR related constants*****************************************/

#define BAR4_SIZE                                 0x00080000
#define FIN_BAR2_SIZE                             TOTAL_FIN_MEM_SIZE
#define WRASSE_BAR2_SIZE                          TOTAL_WRASSE_MEM_SIZE
#define BAR0_SIZE                                 0x00080000

#define GEN_INT_REGS_LEN                          32
#define ENGN_INT_REGS_LEN                         32 

#define BAR_READ_CMD                              "barrd"
#define BAR_WRITE_CMD                             "barwr"

#define BAR_READ                                  1
#define BAR_WRITE                                 0

#define BAR_BUFF_LEN                              BUFF_LEN
#define BAR_COMMAND_LEN                           BUFF_LEN
#define BAR_NUM_PARAMS                            2
#define BAR_CMD_ADDR                              0
#define BAR_CMD_DATA                              1

/****************************FLASH related constants*******************************************/

#define NUM_SECTORS                               4096
#define SECTOR_SIZE                               (64 * 1024)
#define TOTAL_CHIP_SIZE                           (NUM_SECTORS * SECTOR_SIZE)  

#define SPI_PAGE_SIZE                             256
                                                  
#define SOFT_RESET_REG                            0xA940
#define CONTROL_REG                               0xA960
#define GLOBAL_INTERRUPT_REG                      0xA91C
#define INTERRUPT_ENABLE_REG                      0xA928
#define SPI_DTR                                   0xA968
#define SPI_DRR                                   0xA96C
#define SLAVE_SEL_REG                             0xA970
#define INTERRUPT_STATUS_REG                      0xA920
#define TX_FIFO_OCCUPANCY_REG                     0xA974
#define RX_FIFO_OCCUPANCY_REG                     0xA978
#define SPI_STATUS_REG                            0xA964
                                                  
#define PROG_BASE_ADDR_GOLDEN                     0x00
#define PROG_BASE_ADDR_MAIN                       128*1024*1024
#define SOFT_RESET_IP                             0x0A
#define ENABLE_SPI                                0x186
#define ENABLE_GLOBAL_INTERRUPT                   0x80000000
#define FLASH_WRITE_ENABLE                        0x06
#define SELECT_LINE_ENABLE                        0x0
#define ENABLE_IP_SEND                            0x86
#define TOGGLE_CLEAR_DTR                          0x4
#define ERASE_SECTOR                              0xD8
#define DESELECT_SLAVE                            0xFFFFFFFF
#define SET_FIFO_RESET                            0x1E6
#define STATUS_REG                                0x5
#define FLAG_STATUS_REG                           0x70
#define FOUR_BYTE_ADDR                            0xB7
#define ERASE_DIE                                 0xC4
#define PROGRAM_PAGE                              0x2
#define FAST_READ_PAGE                            0x03
                                                  
#define READ_STATUS                               0x5
#define DUMMY_BYTE                                0x00

#define WORKING_IMAGE                             137
#define GOLDEN_IMAGE                              731

#define PROG_STATE_LIST\
    "NO OPERATION",\
    "ERASING",\
    "PROGRAMING",\
    "VERIFYING",\
    "SUCCEEDED",\
    "FAILED",\
    "UNKNOWN"
    
#define FAIL_CODE_LIST\
    "N/A",\
    "IMAGE SIZE NOT PROVIDED",\
    "FLASH LOCKED/PROGRAMING ONGOING",\
    "IMAGE FILE TOO BIG",\
    "FAILED TO PROGRAME PAGE",\
    "FAILED TO ERASE",\
    "DATA VERIFICATION FAILED",\
    "INTERNAL ERROR",\
    "UNKNOWN"

/****************************i2c related constants*********************************************/

#define I2C_CLK_LO                                0x9000
#define I2C_CLK_HI                                0x9004
#define I2C_CTRL                                  0x9008
#define     I2C_CTRL_EIN                          (1 << 6)
#define     I2C_CTRL_EN                           (1 << 7)
#define I2C_TX                                    0x900C
#define     I2C_TX_RD_WR                          (0x01)  /* Read/Write#  */
#define     I2C_TX_ABYTE                          (0xFE)  /* Address byte */
#define I2C_RX                                    0x900C
#define I2C_CMD                                   0x9010
#define     I2C_CMD_IACK                          (1 << 0)
#define     I2C_CMD_ACK                           (1 << 3)
#define     I2C_CMD_WR                            (1 << 4)
#define     I2C_CMD_RD                            (1 << 5)
#define     I2C_CMD_STO                           (1 << 6)
#define     I2C_CMD_STA                           (1 << 7)
#define I2C_STAT                                  0x9010
#define     I2C_STAT_IF                           (1 << 0) /* 'iflag' in struct form */
#define     I2C_STAT_TIP                          (1 << 1)
#define     I2C_STAT_AL                           (1 << 5)
#define     I2C_STAT_BUSY                         (1 << 6)
#define     I2C_STAT_RXACK                        (1 << 7)
#define I2C_MUX_CNT                               0x74
                                                  

#define I2C_MUX_CHAN_1                            0x09
#define I2C_MUX_CHAN_2                            0x0A
#define I2C_MUX_CHAN_3                            0x0B
#define I2C_MUX_CHAN_4                            0x0C
#define I2C_MUX_CHAN_5                            0x0D
#define I2C_MUX_CHAN_6                            0x0E
#define I2C_MUX_CHAN_7                            0x0F

#define I2C_VPD                                   0x52
                                                  
#define I2C_HMC_BASE                              0x10
                                                  
#define ZERO_DEV_OFFSET                           0x0000
#define ZERO_OFFSET_SIZE                          0
                                                  
#define ONE_PAGE                                  1
#define FOUR_PAGES                                4
#define VPD_BD_PG_SIZE                            64
#define VPD_OFFSET_SIZE                           2
#define HMC_OFFSET_SIZE                           4
                                                  
#define I2C_READ                                  1
#define I2C_WRITE                                 0

#define FPGA_I2C_ACCESS                           0x1000, 0x0
#define BOARD_I2C_ACCESS                          0x1000,0x80000000
#define WISHBONE_FREQ                             250000000// 250MHz.  
#define I2C_BUFF_LEN                              BUFF_LEN
#define I2C_COMMAND_LEN                           BUFF_LEN
#define CMD_TYPE_LEN                              6
#define I2C_NUM_PARAMS                            5
#define CMD_BASE_ADDR                             0
#define CMD_DEV_OFF                               1
#define CMD_DATA                                  2
#define CMD_OFFSET_LEN                            3
#define CMD_CHANNEL                               4  

#define CHANNEL_CODE\
    I2C_MUX_CHAN_1,\
    I2C_MUX_CHAN_2,\
    I2C_MUX_CHAN_3,\
    I2C_MUX_CHAN_4,\
    I2C_MUX_CHAN_5,\
    I2C_MUX_CHAN_6,\
    I2C_MUX_CHAN_7
    
#define I2C_READ_CMD                              "i2crd"
#define I2C_WRITE_CMD                             "i2cwr"

/****************************HMC related constants*********************************************/

#define HMC_NUM                                   4
#define MAX_TRIALS                                5 
#define HMC_RESET_ADD                             0x200
#define LINKUP_REG_ADD                            0x100

#define F_H_READ                                  12
#define F_H_SET                                   16

#define HMC_CONTROLLER_STATE_MACHINE_OFFSET       0x8
#define HMC_CONTROLLER_SYS_STATUS_OFFSET          0x4
#define HMC_CONTROLLER_HMC_STATUS_OFFSET          0x14
#define HMC_LINK_STATUS_OFFSET                    0x2
#define HMC_LINK_SYNC_STATUS_OFFSET               0x5
#define LINK_RUN_LENGHT_LIM_REG_OFFSET            0x3
#define LINK_TRAINING_FPGA_OFFSET                 0x10

#define FERR_SIGNALS_LOC                          0x00000084
#define FATAL_ERRORS                              0x000F0000 //19:16 = HMC 3:0

#define TX_TOKEN_OFFSET                           0xB0
#define RESET_HMC_CONT_TX                         0x1

#define BAR4_ADD_CALC_SHIFT                       12

#define HMC_INIT_DONE                             16

#define BAR4_HMC_BASE                             0xB000
#define LINK0_HMC_BASE                            0x00240000
#define LINK1_HMC_BASE                            0x00250000
#define LINK2_HMC_BASE                            0x00260000
#define LINK3_HMC_BASE                            0x00270000

#define GLOBAL_STATUS_REGISTER                    0x00280001

#define LINK2_ACTIVE                              0x4
#define LINK2_RESET                               0x1
#define LINK2_TRAINED                             0x80

#define LINK_ACTIVE_FULLWIDTH                     0xC80 
#define LINK_ACTIVE_HALFWIDTH                     0x480 

#define NVM_ADD                                   0x00280002
#define DISABLE_NVM_WR_STATUS                     0x00000100

#define BEGIN_LINK_TRAINING_ON_HMC                0x800000FF 
#define CHECK_LINK_TRAINING_STATUS                0x002B0004

#define LINK_RUN_LENGTH_LIMIT                     0x00C80000

#define LINK_CONFIG_IN_DEBUG                      0x00000EF9

#define BUFFER_TOKEN_COUNT_REG                    0x00060000
#define BUFFER_SPACE_AVAIL                        0x000000DB

#define VALUE_CONTROL_REG                         0x00108000
#define INIT_DEBUG_VALUE_CONTROL                  0x0000087C

#define ERIDATA0                                  0x002B0000
#define ERIDATA1                                  0x002B0001
#define ERIDATA2                                  0x002B0002
#define ERIDATA3                                  0x002B0003


#define ERIREQ                                    0x002B0004
#define INIT_LINK_CONFIG                          0x813F0005
#define INIT_PHY_CONFIG                           0x80000006

#define tINITCONTI_VIOLATION                      0x48000000
#define ERIREQ_LINK_TRAINING_SUCCESS              0x000000FF
#define LINK_TRAINING_FPGA                        0x2

#define REQ_IDENT_REG                             0x20000
#define CLEAR_CID                                 0xFFFFFFC7


/****************************DEBUG related constants*********************************************/

//These are the verticals 
#define IBMUFTIAN_DBG                             0
#define BOARD_DBG                                 1
#define ENGN_DBG                                  2
#define FLASH_DBG                                 3
#define FPGA_DBG                                  4
#define HMC_DBG                                   5
#define I2C_DBG                                   6
#define INTR_DBG                                  7
#define IOCTL_DBG                                 8
#define PCI_DBG                                   9
#define BAR_DBG                                   10
#define SYSFS_DBG                                 11
#define UTILS_DBG                                 12
#define BD_DBG                                    13
#define MMAP_DBG                                  14
//These are the horizontals
#define LOAD_DBG                                  15
#define FLASHING_DBG                              16
#define DATA_DBG                                  17
#define INT_DBG                                   18
#define FULL_DBG                                  19
#define ERR_DBG                                   20

#define TOTAL_DBG                                 21 // Change this number when adding a dbg area.
#endif
