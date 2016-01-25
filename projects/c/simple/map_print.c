#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main (void)
{
	pid_t pid = getpid ();
	int fd;
	char buff[32];

	snprintf (buff, sizeof (buff), "/proc/%d/maps", pid);

	fd = open (buff, O_RDONLY);

	while (read(fd, buff, 1) == 1)
		putchar (*buff);

	close (fd);
	return 0;
}




