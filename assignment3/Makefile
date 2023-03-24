all: test.o userfs.o
	gcc test.o userfs.o

debug:
	clang test.c userfs.c -I ../utils -g -fsanitize=address -o ./a.out

test.o: test.c
	gcc -c test.c -o test.o -I ../utils

userfs.o: userfs.c
	gcc -c userfs.c -o userfs.o
