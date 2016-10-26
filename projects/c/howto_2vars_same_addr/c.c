/* Basic library. Part of API used tiny working area */

#include "g.h"

#ifdef NPS
#include "c_inl.h"
#endif

#include "c_api.h"

#include <stdio.h>
#include <string.h>

void c_init(void)
{
	printf("%s:>>>\n", __func__);
	memset(&C_WA, 0x42, sizeof(C_WA));
	printf("%s:<<<\n", __func__);
}

void c_foo(void)
{
	printf("%s:enter, WA %p (%d)\n", __func__, &C_WA, C_WA_SZ);
	printf("change C_WA.x from %d ", C_WA.cx);
	printf("to %d\n", C_WA.cx=44);
}

