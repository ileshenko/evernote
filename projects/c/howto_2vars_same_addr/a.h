


#define ASMNAME(x) ASMNAME_(__USER_LABEL_PREFIX__, #x)
#define ASMNAME_(x,y) ASMNAME__(x, y)
#define ASMNAME__(x,y) __asm__(#x y)
int x;

struct b_wa {
	int x;
	int y;
};
