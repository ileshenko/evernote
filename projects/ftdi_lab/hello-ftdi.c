/* hello-ftdi.c: flash LED connected between CTS and GND.
   This example uses the libftdi API.
   Minimal error checking; written for brevity, not durability. */

#include <stdio.h>
#include <ftdi.h>

#define PIN_TX  0x01  /* Orange wire on FTDI cable */
#define PIX_RX  0x02  /* Yellow */
#define PIN_RTS 0x04  /* Green */
#define PIN_CTS 0x08  /* Brown */
#define PIN_DTR 0x10
#define PIN_DSR 0x20
#define PIN_DCD 0x40
#define PIN_RI  0x80


#define LED PIN_RI

int main()
{
    unsigned char c = 0;
    int i;
    struct ftdi_context ftdic;

    /* Initialize context for subsequent function calls */
    ftdi_init(&ftdic);
    printf("\n===== %d\n",ftdic.module_detach_mode);
//    ftdic.module_detach_mode = DONT_DETACH_SIO_MODULE;


    /* Open FTDI device based on FT232R vendor & product IDs */
    if((i = ftdi_usb_open(&ftdic, 0x0403, 0x6001)) < 0)
    {
        printf("Can't open device, (%d) - %m\n", i);
        return 1;
    }

#if 1

    /* Enable bitbang mode with a single output line */
//    ftdi_enable_bitbang(&ftdic, LED);
    ftdi_set_bitmode(&ftdic, LED, BITMODE_BITBANG);
//    ftdi_set_bitmode(&ftdic, LED, BITMODE_CBUS);

    /* Endless loop: invert LED state, write output, pause 1 second */
    for(i = 0; i < 17; i++) {
        c ^= LED;
        ftdi_write_data(&ftdic, &c, 1);
        sleep(1);
    }
    ftdi_disable_bitbang(&ftdic);
#endif
    ftdi_usb_close(&ftdic);
    ftdi_deinit(&ftdic);

    return 0;
}
