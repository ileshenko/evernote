/* Main project header file */

#ifndef _A_TYPES_H_
#define _A_TYPES_H_

/* For external usage */
#define A_WA_SZ sizeof(struct a_wa)

/* For internal usage */
struct a_wa {
	int ax;
	int ay;
};

#define A_WA (*(struct a_wa *)g_was)

#endif
