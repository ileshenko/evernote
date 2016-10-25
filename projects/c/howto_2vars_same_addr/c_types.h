/* Project C api */

#ifndef _C_H_
#define _C_H_

struct c_wa {
	int cx;
	int cy;
	char carr[42];
};

#define C_WA_SZ sizeof(struct c_wa)
#define C_WA (*(struct c_wa *)g_was)

struct c_tiny_wa {
	char t_a;
	char t_b;
};

extern struct c_tiny_wa *g_ct_wa;
#define CT_WA (*g_ct_wa)

void c_foo(void);
void c_bar(void);

void ct_foo(void);
void ct_bar(void);

#endif
