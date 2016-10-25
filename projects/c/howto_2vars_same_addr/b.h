/* Project B api */

#ifndef _B_H_
#define _B_H_

#include "c.h"

struct b_wa {
	int bx;
	int by;
	char barr[124];
	struct c_tiny_wa tiny_wa;
};

#define B_WA_SZ sizeof(b_wa)

//extern struct b_wa g_bwa;
extern struct b_wa g_bwa ASMNAME(my_cmem.bwa);

void b_foo(void);
static void b_foo_inline(void);


#endif
