#define ASMNAME(x) ASMNAME_(__USER_LABEL_PREFIX__, #x)
#define ASMNAME_(x,y) ASMNAME__(x, y)
#define ASMNAME__(x,y) __asm__(#x y)
int x;
extern int y ASMNAME(x);

#include <stdio.h>

int main(void)
{
    printf("=== %d ===\n",(&x == &y)); /* will exit successfully */
    y = 42;
    x = 123;
    printf("%d\n", y);

	return 0;
}
