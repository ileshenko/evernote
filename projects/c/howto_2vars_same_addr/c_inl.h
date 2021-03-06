/* Project C inline implementations */

#ifndef _C_INL_H_
#define _C_INL_H_

#include "c_types.h"

#include <stdio.h>

__always_inline static void c_bar(void)
{
	printf("%s:>>>>> WA %p (%lu)\n", __func__, &C_WA, C_WA_SZ);
	printf("change C_WA.x from %d ", C_WA.cx);
	printf("to %d\n", C_WA.cx=1005);
	printf("%s:<<<<<\n", __func__);
}

__always_inline static void ct_foo(void)
{
	printf("%s:DDDDDDD WA %p (%lu)\n", __func__, &CT_WA, sizeof(CT_WA));
	printf("change CT_WA.t_a from %d ", CT_WA.t_a);
	printf("to %d\n", CT_WA.t_a=99);
	printf("%s:dddddd\n", __func__);
}

__always_inline static void ct_bar(void)
{
	printf("%s:DDDDDDD WA %p (%lu)\n", __func__, &CT_WA, sizeof(CT_WA));
	printf("change CT_WA.t_a from %d ", CT_WA.t_a);
	printf("to %d\n", CT_WA.t_a=36);
	printf("%s:dddddd\n", __func__);
}

#endif

