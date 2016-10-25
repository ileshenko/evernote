/* This are global definitions for our project */

#ifndef _G_H_
#define _G_H_

#include <stdint.h>

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MAX3(a,b,c) (MAX(a,b)>(c) ? MAX(a,b) : (c))
#define MAX4(a,b,c,d) (MAX3(a,b,c)>(d) ? MAX3(a,b,c) : (d))

extern uint8_t g_was[];

#endif
