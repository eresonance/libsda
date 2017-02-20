
all:
	gcc -g -posix -Wall -Wextra -Wpointer-arith -Wno-sign-compare -Werror -DSDA_TEST_MAIN -o sda_test sda.c && ./sda_test.exe
