
WARNINGS:= -Wall -Wextra -Wpointer-arith -Wno-sign-compare -Wcast-align -Werror

all: sda_test.exe

sda_test.exe:
	gcc -g -posix ${WARNINGS} -DSDA_TEST_MAIN -o sda_test sda.c && ./sda_test.exe

drmemory: sda_test.exe
	/c/usr/drmemory/bin/drmemory.exe -v sda_test.exe

.PHONY:=all drmemory