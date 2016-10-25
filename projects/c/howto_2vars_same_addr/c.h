/* Project C api */

#ifndef _C_H_
#define _C_H_

struct c_wa {
	int cx;
	int cy;
	char carr[42];
};

#define C_WA_SZ sizeof(c_wa)

struct c_tiny_wa {
	char t_a;
	char t_b;
};

void c_foo(void);
static void c_foo_inline(void);

void c_tiny(void);
static void c_tiny_inline(void);

#endif
