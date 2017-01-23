/* Main project header file */

#ifndef _A_INL_H_
#define _A_INL_H_

#include "b_api.h"

#include "a_types.h"

__always_inline static void a_bar(void)
{
	printf("%s:>>>>> WA %p    (%lu)\n", __func__, &A_WA, A_WA_SZ);
	printf("change A_WA.x from %d ", A_WA.ax);
	printf("to %d\n", A_WA.ax=101);

	printf("======================\n");
	b_bar();
	printf("%s:<<<<<\n", __func__);

}

#endif
