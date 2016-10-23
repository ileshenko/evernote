/* Project B inline implementations */

#ifndef _B_INL_H_
#define _B_INL_H_


static inline void b_foo_inline(void)
{
	printf("%s:", __func__);
	printf("B_C.bx from %d to 11\n", B_C.bx);
	B_C.bx = 11;
}


#endif
