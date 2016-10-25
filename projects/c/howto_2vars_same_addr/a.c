/* Main project file */

#include <stdio.h>

#include "a.h"
#include "b.h"


struct a_wa {
	int a;
	int b;
};

struct {
	int a;
	union {
		struct a_wa awa;
		struct b_wa bwa;
	};
} my_cmem;

extern struct b_wa g_bwa ASMNAME(my_cmem.bwa);

int main(void)
{
	my_cmem.awa.a =  123;
	printf("was my_cmem.awa.a = %d\n", my_cmem.awa.a);
	b_foo();
	printf("now my_cmem.awa.a = %d\n", my_cmem.awa.a);
	printf("now g_bwa.x = %d\n", g_bwa.x);

	return 0;
}
