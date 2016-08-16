#include <stdio.h>

enum {
	r, g, v
};

char *clr = "rgv";

enum {
	o, d, w
};

char *shape = "odw";

enum {
	s, e, l
};

char *fill = "sel";

int arr[][4] = {
	{3,v,d,e},
	{2,g,o,e},
	{3,v,w,s},
	{1,g,w,l},
	{2,v,o,e},
	{1,v,w,e},
	{1,g,o,l},
	{2,v,d,l},
	{3,r,o,e},
	{1,v,o,l},
	{1,g,w,s},
	{1,r,o,e},
};

static void printset(int a[4])
{
	printf("%d%c%c%c ", a[0], clr[a[1]], shape[a[2]], fill[a[3]]);
}

static void test_set(int a[4], int b[4], int c[4])
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if (a[i] == b[i] && a[i] == c[i])
			continue;
		if (a[i] != b[i] && a[i] != c[i] && b[i] != c[i])
			continue;
		return;
	}

	printf("set ");
	printset(a);
	printset(b);
	printset(c);
	printf("\n");
}

int main(void)
{
	int i,j,k;

	for (i = 0; i < 12-2; i++)
	{
		for (j = i+1; j< 12-1; j++)
		{
			for (k = j+1; k < 12; k++)
				test_set(arr[i], arr[j], arr[k]);
		}
	}

	return 0;
}

