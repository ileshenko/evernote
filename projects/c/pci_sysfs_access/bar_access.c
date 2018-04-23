/* http://billfarrow.blogspot.co.il/2010/09/userspace-access-to-pci-memory.html

fd = open("/sys/devices/pci0001\:00/0001\:00\:07.0/resource0", O_RDWR | O_SYNC);
ptr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
printf("PCI BAR0 0x0000 = 0x%4x\n",  *((unsigned short *) ptr);
*/

/* hw116
 * lspci -Dtv
 * sudo dd bs=4 count=1 if=/sys/devices/pci0000\:80/0000\:80\:02.0/0000\:82\:00.0/0000\:83\:08.0/0000\:84\:00.0/resource0 of=/tmp/qqq
 */

/* https://github.com/billfarrow/pcimem */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


#define BAR0_FNAME "/sys/devices/pci0000:80/0000:80:02.0/0000:82:00.0/0000:83:08.0/0000:84:00.0/resource0"
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

int main(void)
{
	void *map_base;
	int fd;
	uint32_t r_result;

	fd = open(BAR0_FNAME, O_RDWR | O_SYNC);
	if (fd == -1)
	{
		perror("Error open sys file:");
		exit(-1);
	}

    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_base == (void *) -1)
	{
		perror("Error mmap BAR0:");
		exit(-1);
	}

	r_result = *(uint32_t *)map_base;
	printf("r_result 0 %#010x\n", r_result);
	
//	*(uint32_t *)map_base = 0xc001babe;
	*(uint32_t *)map_base = 0x60000000;
//	*(uint32_t *)map_base = 0x02000000;
//	*(uint32_t *)map_base = 8;
//	*(uint32_t *)map_base = 0xdeadbeef;
//	*(uint32_t *)map_base = 0xc01dbaba;

	r_result = *(uint32_t *)map_base;
	printf("r_result 1 %#010x\n", r_result);
//	r_result = *(uint32_t *)map_base;
//	printf("r_result 2 %#010x\n", r_result);
//	r_result = *(uint32_t *)map_base;
//	printf("r_result 3 %#010x\n", r_result);
	r_result = *(uint32_t *)map_base;
	printf("r_result 4 %#010x\n", r_result);

	if (munmap(map_base, MAP_SIZE) == -1)
	{
		perror("Error munmap BAR0:");
		exit(-1);
	}
    close(fd);

	return 0;
}
