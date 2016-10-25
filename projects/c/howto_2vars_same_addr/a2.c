/* Main project. Dedicated functions */

#include "g.h"

#include "a_api.h"
#include "b_api.h"

#include <stdio.h>
#include <string.h>

void a_init(void)
{
	printf("%s:>>>\n", __func__);
	memset(&A_WA, 0xeb, sizeof(A_WA));
	b_init();
	printf("%s:<<<\n", __func__);
}
