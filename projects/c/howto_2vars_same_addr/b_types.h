/* Project B api */

#ifndef _B_TYPES_H_
#define _B_TYPES_H_

#include "c_types.h"

/* For external usage */
#define B_WA_SZ sizeof(struct b_wa)

/* For internal usage */
struct b_wa {
	int bx;
	int by;
	char barr[124];
	uint8_t c_tiny_wa[CT_WA_SZ];
};

#define B_WA (*(struct b_wa *)g_was)

#endif
