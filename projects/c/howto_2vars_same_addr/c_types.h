/* Project C api */

#ifndef _C_TYPES_H_
#define _C_TYPES_H_

/* For external usage */
#define C_WA_SZ sizeof(struct c_wa)
#define CT_WA_SZ sizeof(struct c_tiny_wa)

/* For internal usage */
struct c_wa {
	int cx;
	int cy;
	char carr[42];
};

#define C_WA (*(struct c_wa *)g_was)

struct c_tiny_wa {
	char t_a;
	char t_b;
};

extern uint8_t *g_ct_wa;
#define CT_WA (*(struct c_tiny_wa *)g_ct_wa)

#endif
