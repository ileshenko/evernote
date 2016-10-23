/* Project C inline implementations */

#ifndef _C_INL_H_
#define _C_INL_H_

static inline void c_foo_inline(void)
{
	printf("%s:", __func__);
	printf("C_C.cx from %d to 36\n", C_C.cx);
	C_C.cx = 36;
}


#endif

