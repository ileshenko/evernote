/* Main project file */
/* comilation for NPS
 *    cc -DNPS -o demo a.c a2.c b.c c.c && ./demo
 * Compilation for X86
 *    cc -DX86 -o demo a.c a2.c b.c c.c && ./demo
 */

#include <stdio.h>

#include "g.h"

#ifdef NPS
#include "c_inl.h"
#include "b_inl.h"
#include "a_inl.h"
#endif

#include "c_api.h"
#include "b_api.h"
#include "a_api.h"

uint8_t *g_ct_wa; /* may be inside c.c */
uint8_t d;
uint8_t g_was[MAX3(A_WA_SZ, B_WA_SZ, C_WA_SZ)];

static void a_foo(void)
{
	printf("%s:enter, WA %p (%lu)\n", __func__, &A_WA, A_WA_SZ);
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
