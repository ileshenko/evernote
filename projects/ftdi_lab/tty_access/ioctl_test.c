
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

//-----------------------
#include <ftdi.h>

int ftd_set_bitmode(struct ftdi_context *ftdi, unsigned char bitmask, unsigned char mode)
{
    unsigned short usb_val;

    if (ftdi == NULL || ftdi->usb_dev == NULL)
        ftdi_error_return(-2, "USB device unavailable");

    usb_val = bitmask; // low byte: bitmask
    usb_val |= (mode << 8);
    if (usb_control_msg(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE, SIO_SET_BITMODE_REQUEST, usb_val, ftdi->index, NULL, 0, ftdi->usb_write_timeout) != 0)
        ftdi_error_return(-1, "unable to configure bitbang mode. Perhaps selected mode not supported on your chip?");

    ftdi->bitbang_mode = mode;
    ftdi->bitbang_enabled = (mode == BITMODE_RESET) ? 0 : 1;
    return 0;
}

//----------------------

/*
 http://www.cmrr.umn.edu/~strupp/serial.html#5_1_1
TIOCM_LE	DSR (data set ready/line enable)
TIOCM_DTR	DTR (data terminal ready)
TIOCM_RTS	RTS (request to send)
TIOCM_ST	Secondary TXD (transmit)
TIOCM_SR	Secondary RXD (receive)
TIOCM_CTS	CTS (clear to send)
TIOCM_CAR	DCD (data carrier detect)
TIOCM_CD	Synonym for TIOCM_CAR
TIOCM_RNG	RNG (ring)
TIOCM_RI	Synonym for TIOCM_RNG
TIOCM_DSR	DSR (data set ready)

*/
int main(void)
{
    int fd, i;
    int status = 0;

    fd = open("/dev/JLabI001", O_RDWR);
    printf("fd=%d\n", fd);
    printf("ioctl=%d\n", ioctl(fd, TIOCMGET, &status));

    printf("status=%#010x\n", status);

    for (i = 0; i < 500; i++)
    {
	status ^= TIOCM_DTR | TIOCM_RTS;
	usleep(4000);
    	ioctl(fd, TIOCMSET, &status);
    }
    for (i = 0; i < 1000; i++)
    {
	status ^= TIOCM_DTR | TIOCM_RTS;
	usleep(50);
    	ioctl(fd, TIOCMSET, &status);
    }
    printf("status=%#010x\n", status);
    ioctl(fd, TIOCMGET, &status);
    printf("status=%#010x\n", status);


//    printf("ioctl=%d\n%m\n", ioctl(fd, TIOCMSET, &status));

    sleep(2);
    close(fd);
    return 0;
}
