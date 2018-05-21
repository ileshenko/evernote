#include "ibmuftian_pci.h"
#include "ibmuftian.h"

pci_ers_result_t ibmuftian_pci_error(struct pci_dev *, pci_channel_state_t);
pci_ers_result_t ibmuftian_pci_slot_reset(struct pci_dev *);
void ibmuftian_pci_resume(struct pci_dev *);
int ibmuftian_pci_probe(struct pci_dev *, const struct pci_device_id *);
void ibmuftian_pci_close(struct pci_dev *);
int pci_probe(struct pci_dev *, const struct pci_device_id *);
void ibmuftian_pci_cleanup(struct pci_dev *);


struct pci_device_id ibmuftian_id_table[] =
{
    { PCI_DEVICE(IBMUFTIAN_PCI_VENDOR_ID, IBMUFTIAN_PCI_FIN_DEVICE_ID) },
    { ZERO }
};

MODULE_DEVICE_TABLE(pci, ibmuftian_id_table);

static struct pci_error_handlers ibmuftian_pci_err_handler =
{
    .error_detected = ibmuftian_pci_error,
    .slot_reset     = ibmuftian_pci_slot_reset,
    .resume         = ibmuftian_pci_resume,
};

static struct pci_driver ibmuftian_pci = 
{
    .name        = IBMUFTIAN_NAME,
    .id_table    = ibmuftian_id_table,
    .probe       = pci_probe,
    .remove      = ibmuftian_pci_cleanup,
    .err_handler = &ibmuftian_pci_err_handler,
};




pci_ers_result_t ibmuftian_pci_error(struct pci_dev *pdev, pci_channel_state_t state)
{
    pci_disable_device(pdev);

   PRINT(DBG_PRAM({PCI_DBG}),"PCI Error detected, channel state: %s",
           (state == pci_channel_io_normal ?        "Normal" :
           (state == pci_channel_io_frozen ?        "Frozen" :
           (state == pci_channel_io_perm_failure ?  "Failure":
                                                    "Unknown"))));
    return PCI_ERS_RESULT_NEED_RESET;
}

pci_ers_result_t ibmuftian_pci_slot_reset(struct pci_dev *pdev)
{
    PRINT(DBG_PRAM({PCI_DBG}),"PCI Slot reset detected.");

    if (pci_enable_device(pdev))
    {
        PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),"PCI Error, Cannot re-enable PCI device after reset.");
        return PCI_ERS_RESULT_DISCONNECT;
    }
    pci_set_master(pdev);

    return PCI_ERS_RESULT_RECOVERED;
}

void ibmuftian_pci_resume(struct pci_dev *pdev)
{
    PRINT(DBG_PRAM({PCI_DBG}),"PCI IO Resume detected.");
    pci_enable_wake(pdev, 0, 0);
}

int ibmuftian_pci_init(struct pci_dev *pdev, const char *name, int *link_width, void *dev)
{
    int res = 0, pos = 0;
    u16 reg16 = 0;
	
	int maxvec = 0;

    if (unlikely(!pdev || !name || !link_width)) goto exit;

    // Enable the memory resources
    if ((res = pci_enable_device(pdev)))
    {
        dev_err(&pdev->dev, "Failed to enable PCI device\n");
        goto exit;
    }

    // Request memory regions
    if ((res = pci_request_regions(pdev, IBMUFTIAN_NAME)) < 0)
    {
        dev_err(&pdev->dev, "Failed to get PCI memory region\n");
        goto exit_disable_dev;
    }
    pci_write_config_word(pdev, PCI_COMMAND, PCI_COMMAND_MEMORY); // Enable response in Memory space
    pci_set_master(pdev);                                         // Set PCI master
    pci_intx(pdev, 0);                                            // Disable INTx support (only MSI)

    // Test MSI support.
    if (!pci_find_capability(pdev, PCI_CAP_ID_MSI))
    {
        PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),"PCIe device does not support MSI!");
        dev_err(&pdev->dev, "Failed to enable MSI\n");
        goto exit_release_mem;
    }

	
    maxvec=pci_msi_vec_count(pdev);
    PRINT(DBG_PRAM({PCI_DBG}),"FPGA requested %d MSI vectors, from PCIe cfg reg 0x%x\n",
           maxvec,
           pos +
           PCI_MSI_FLAGS);

    if ((res = pci_enable_msi_exact(pdev, maxvec)))
    {
        PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),"Failed to enable %d MSI vectors, %d were possible. Trying to enable 1\n",
               maxvec,
               res);
        if ((res = pci_enable_msi(pdev)))
        {
            PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),"Failed to enable MSI: %d\n", res);
            goto exit_release_mem;
        }
    }

    // Configure PCIe endpoint options...
    pos = 0;
    pos = pci_find_capability(pdev, PCI_CAP_ID_EXP);
    //if (!pos) goto exit_free_msi_addr;

    pci_read_config_word(pdev, pos + PCI_EXP_DEVSTA, &reg16);
    reg16 |= PCI_EXP_DEVSTA_CED;    // Correctable Error Detected
    reg16 |= PCI_EXP_DEVSTA_NFED;   // Non-Fatal Error Detected
    reg16 |= PCI_EXP_DEVSTA_FED;    // Fatal Error Detected
    reg16 |= PCI_EXP_DEVSTA_URD;    // Unsupported Request Detected
    reg16 |= PCI_EXP_DEVSTA_AUXPD;  // AUX Power Detected
    reg16 |= PCI_EXP_DEVSTA_TRPND;  // Transactions Pending
    pci_write_config_word(pdev, pos + PCI_EXP_DEVSTA, reg16);

    pci_read_config_word(pdev, pos + PCI_EXP_DEVCTL, &reg16);
    reg16 &= ~PCI_EXP_DEVCTL_NOSNOOP_EN;
    reg16 &= ~PCI_EXP_DEVCTL_PHANTOM;
    reg16 &= ~PCI_EXP_DEVCTL_RELAX_EN;
    reg16 &= ~PCI_EXP_DEVCTL_EXT_TAG;
    pci_write_config_word(pdev, pos + PCI_EXP_DEVCTL, reg16);

    // Clear errors
    reg16  = PCI_EXP_DEVSTA_CED;    // Correctable Error Detected
    reg16 |= PCI_EXP_DEVSTA_NFED;   // Non-Fatal Error Detected */
    reg16 |= PCI_EXP_DEVSTA_FED;    // Fatal Error Detected */
    reg16 |= PCI_EXP_DEVSTA_URD;    // Unsupported Request Detected */
    reg16 |= PCI_EXP_DEVSTA_AUXPD;  // AUX Power Detected */
    reg16 |= PCI_EXP_DEVSTA_TRPND;  // Transactions Pending */
    pci_write_config_word(pdev, pos + PCI_EXP_DEVSTA, reg16);

    // Test link width
    pci_read_config_word(pdev, pos + PCI_EXP_LNKSTA, &reg16);
    switch ((reg16 >> PCI_EXP_LNKSTA_NLW_SHIFT) & PCI_EXP_LNKSTA_NLW)
    {
#define FMT(x_) "PCIe Link width not x8, negotiated width " # x_

        case 0x01: *link_width = 1;  PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}), FMT(x1));        
        break;
        case 0x02: *link_width = 2;  PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),FMT(x2));
        break;
        case 0x04: *link_width = 4;  PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),FMT(x4));
        break;
        case 0x08: *link_width = 8;  /* good, don't print */   
        break;
        case 0x0C: *link_width = 12; PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),FMT(x12)); 
        break;
        case 0x10: *link_width = 16; PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),FMT(x16));
        break;
        case 0x20: *link_width = 32; PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),FMT(x32)); 
        break;
        default:   *link_width = 0;  PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),FMT(Undefined));
        break;

#undef FMT
    }
    return 0;

exit_release_mem:
    pci_release_regions(pdev);

exit_disable_dev:
    pci_disable_device(pdev);

exit:
	PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),"Failed pci init\n");
    return 1;
}

void ibmuftian_pci_destroy(struct pci_dev *pdev, void *dev)
{
    synchronize_irq(pdev->irq);
    free_irq(pdev->irq, dev);
	pci_disable_msi(pdev);
    pci_release_regions(pdev);
    pci_disable_device(pdev);
}

void ibmuftian_pci_unregister(void)
{
    pci_unregister_driver(&ibmuftian_pci);
}

int ibmuftian_pci_register(void)
{
    int err;
    
    PRINT(DBG_PRAM({PCI_DBG}),"Registering PCI devices");
    
    if(unlikely(err = pci_register_driver(&ibmuftian_pci)))
    {
        return err;
    }
    PRINT(DBG_PRAM({PCI_DBG}),"Done registering PCI devices");
    return 0;
}

int pci_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
{
    int err;
    
    if(unlikely(err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(DMA_BIT_MASK_LEN))))
    {
        
        PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),"ERROR SETTING UP DMA MASKS");
        dev_err(&pdev->dev, "%s: Error %d setting up DMA mask\n", __func__, err);
    }
    
    if(create_board(pdev))
    {
        
        PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),"failed to create/init driver");        
        return FAILURE;
    }

    return SUCCESS;
}

void ibmuftian_pci_cleanup(struct pci_dev *pdev)
{
    ibmuftian_board *board = (ibmuftian_board *)pci_get_drvdata(pdev);
    if(!board)
    {
        PRINT(DBG_PRAM({PCI_DBG, ERR_DBG}),"failed to get board pointer");
        return;
    }

    board_delete(board);
}
