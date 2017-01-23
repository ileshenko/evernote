/* Project B inline implementations */

#ifndef _B_INL_H_
#define _B_INL_H_

#include "b_types.h"
#include "c_api.h"

#include <stdio.h>

__always_inline static void b_bar(void)
{
	printf("%s:>>>>> WA %p    (%lu)\n", __func__, &B_WA, B_WA_SZ);
	printf("change B_WA.x from %d ", B_WA.bx);
	printf("to %d\n", B_WA.bx=1001);

	printf("++++++++++++++++++++++\n");
	c_bar();
	printf("~~~~~~~~~~~~~~~~~~~~~~\n");
	ct_bar();
	printf("%s:<<<<<\n", __func__);
}

#endif
