#CFLAGS=-Wall -Wextra -pedantic-errors -Werror

%.o: %.c
	gcc -c -o $@ $(LIBS) $(CFLAGS) $^

var3: hash17_2.o hash17_parser_input.o var2.o var3.o
	gcc -o $@ $^

var2: hash17_2.o hash17_parser_input.o var2.o
	gcc -o $@ $^

var1: hash17_2.o hash17_parser_input.o var1.o
	gcc -o $@ $^

