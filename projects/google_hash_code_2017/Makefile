#CFLAGS=-Wall -Wextra -pedantic-errors -Werror

%.o: %.c
	gcc -c -o $@ $(LIBS) $(CFLAGS) $^

var2: hash17_2.o hash17_parser_input.o
	gcc -o $@ $^


