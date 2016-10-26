/* Main project. Dedicated functions */

#include "g.h"

#include "b_api.h"
#include "a_api.h"

#include <stdio.h>
#include <string.h>

void a_init(void)
{
	printf("%s:>>>\n", __func__);
	memset(&A_WA, 0xeb, sizeof(A_WA));
	b_init();
	printf("%s:<<<\n", __func__);
}

#ifdef X86
/* For demo purpoces use all inline functions as global functions */
#undef static
#define static
#include "c_inl.h"
#include "b_inl.h"
#include "a_inl.h"
#endif

