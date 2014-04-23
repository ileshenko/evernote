#include <stdio.h>

int main(int argc, char **argv)
{
    int i;
    unsigned int val;

    argc--;
    argv++;
    if (!argc)
    {
	printf("Usage: bits hex-val\n");
	return 1;
    }

    printf("\t\t");
    for (i = 31; i >= 0; i--)
    {
	printf(" %2d", i);
	if (i && !(i&0x7))
	    printf("  | ");
    }
    printf("\n");

    while (argc--)
    {
	sscanf(*argv, "%x", &val);
	argv++;
	printf("%0#10x\t", val);

	for (i = 31; i >= 0; i--)
	{
	    printf("  %c", val&0x80000000 ? '*' : ' ');
	    val <<= 1;
	    if (i && !(i&0x7))
		printf("  | ");
	}
	printf("\n");
    }
    return 0;
}
