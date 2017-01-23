
#include "g.h"

#ifdef NPS
#include "c_inl.h"
#include "b_inl.h"
#endif

#include "c_api.h"
#include "b_api.h"

#include <stdio.h>
#include <string.h>

void b_init(void)
{
	printf("%s:>>>\n", __func__);

	g_ct_wa = B_WA.c_tiny_wa; /* Important Init before usage C tiny API */

	memset(&B_WA, 0xca, sizeof(B_WA));
	c_init();

	printf("%s:<<<\n", __func__);
}

void b_foo(void)
{
	printf("%s:enter, WA %p (%lu)\n", __func__, &B_WA, B_WA_SZ);
	printf("change B_WA.x from %d ", B_WA.bx);
	printf("to %d\n", B_WA.bx=33);
	ct_foo();
}

