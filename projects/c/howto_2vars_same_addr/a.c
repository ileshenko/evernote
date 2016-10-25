/* Main project file */

#include <stdio.h>

#include "g.h"

#include "c_inl.h"
#include "b_inl.h"
#include "a_inl.h"

#include "c_api.h"
#include "b_api.h"
#include "a_api.h"

struct c_tiny_wa *g_ct_wa;
uint8_t g_was[MAX3(A_WA_SZ, B_WA_SZ, C_WA_SZ)];

static void a_foo(void)
{
	printf("%s:enter, WA %p (%d)\n", __func__, &A_WA, A_WA_SZ);
	printf("change A_WA.x from %d ", A_WA.ax);
	printf("to %d\n", A_WA.ax=22);
}

int main(void)
{
	a_init(); /* Calls once used code */
	printf("----------------------------------------\n");

	a_foo();
	printf("----------------------------------------\n");
	a_bar();
	printf("----------------------------------------\n");
	b_foo();
	printf("----------------------------------------\n");
	c_foo();
	printf("----------------------------------------\n");
	b_bar();
	printf("----------------------------------------\n");
	c_bar();

	return 0;
}
