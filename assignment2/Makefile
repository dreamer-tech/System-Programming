all:
	gcc parser.c execute.c shell.c main.c -o tsh

debug:
	clang parser.c execute.c shell.c main.c -g -fsanitize=address -o tsh