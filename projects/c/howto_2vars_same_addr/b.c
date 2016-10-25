#include <stdio.h>

#include "g.h"
#include "b_inl.h"
#include "b_api.h"

#include <string.h>

void b_init(void)
{
	printf("%s:>>>\n", __func__);
	memset(&B_WA, 0xca, sizeof(B_WA));
	g_ct_wa = &B_WA.tiny_wa;
	c_init();
	printf("%s:<<<\n", __func__);
}

void b_foo(void)
{
	printf("%s:enter, WA %p (%d)\n", __func__, &B_WA, B_WA_SZ);
	printf("change B_WA.x from %d ", B_WA.bx);
	printf("to %d\n", B_WA.bx=33);
	ct_foo();
}

