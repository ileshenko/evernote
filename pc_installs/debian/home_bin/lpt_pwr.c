/* Prototype from http://tldp.org/HOWTO/IO-Port-Programming.html#toc6 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/io.h>

#define BASEPORT 0x378 /* lp1 */

static int usage(void)
{
    printf("Use lpt_pwr on|off|reset\n");
    return -1;
}

static void pwr(int is_on)
{
    /* Get access to the ports */
    if (ioperm(BASEPORT, 3, 1))
    {
	perror("ioperm");
	exit(1);
    }
  
    /* Set the data signals (D0-7) of the port to all low (0) */
    outb(is_on ? 0 : 1, BASEPORT);
  
    /* We don't need the ports anymore */
    if (ioperm(BASEPORT, 3, 0))
    {
	perror("ioperm");
	exit(1);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
	return usage();

    if (!strcasecmp(argv[1], "on"))
    	pwr(1);
    else if (!strcasecmp(argv[1], "off"))
	pwr(0);
    else if (!strcasecmp(argv[1], "reset"))
    {
	pwr(0);
	sleep(2);
	pwr(1);
    }
    else
	return usage();

    return 0;
}

