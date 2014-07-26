
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>


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

    for (i = 0; i < 1000; i++)
    {
	status ^= TIOCM_RTS;
    	printf("status=%#010x\n", status);
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
