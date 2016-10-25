/* Project B api */

#ifndef _B_H_
#define _B_H_

#include "c_types.h"

struct b_wa {
	int bx;
	int by;
	char barr[124];
	struct c_tiny_wa tiny_wa;
};

#define B_WA_SZ sizeof(struct b_wa)
#define B_WA (*(struct b_wa *)g_was)
#define G_CT_WA (B_WA.tiny_wa)

#endif
