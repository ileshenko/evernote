#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
	printf("Need one parameter at least\n");
	return -1;
    }

#ifndef __CYGWIN__
    if (setuid(0))
    {
	perror("Cannot setuid()");
	return 1;
    }

    if (setgid(0))
    {
	perror("Cannot setgid()");
	return 2;
    }
#endif

    execvp(argv[1], argv+1);
    perror(argv[1]);
    return 3;
}

