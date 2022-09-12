
#include <stdio.h>
#include <stdlib.h>


#define m(width) ((1<<(width))-1)
#define f(val, width, offset) ((val>>(offset)) & ((1<<(width))-1))


static void printul(char *valul)
{
	union {
		unsigned long arg;
		unsigned char bytes[8];
	} val;
	int i;

	val.arg = strtoul(valul, NULL, 16);

	for (i = 7; i >= 0; i--)
		printf("%02x ", val.bytes[i]);
	printf("  |  ");

	printf("%1x ", f(val.arg, 4, 60));
	printf("%1x ", f(val.arg, 4, 56));
	printf("%1x ", f(val.arg, 4, 52));
	printf("%4x ", f(val.arg, 16, 36));
	printf("%2x ", f(val.arg, 6, 30));
	printf("%1x ", f(val.arg, 2, 28));
	printf("%7x ", f(val.arg, 28, 0));
	printf("\n");

}

int main(int argc, char *argv[])
{
	int i;


	for (i = 1; i < argc; i++)
		printul(argv[i]);

	return 0;
}

