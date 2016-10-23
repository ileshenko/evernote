#include <stdio.h>
#include "c.h"

//extern int y;
//extern int y ASMNAME(x);

void c_foo(void)
{
//	printf("y = %d\n", y);
//	y = 99;
//	printf("y = %d\n", y);
	printf("b_foo: q_bwa.x was %d\n", g_bwa.x);
	g_bwa.x = 42;
	printf("b_foo: q_bwa.x become %d\n", g_bwa.x);
}


