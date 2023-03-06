CC=gcc
CFLAGS=-g -std=c11 #-fno-stack-protector 

TOKENIZE_OBJS=$(patsubst %.c,%.o,$(filter-out shell.c,$(wildcard *.c)))
SHELL_OBJS=$(patsubst %.c,%.o,$(filter-out tokenize.c,$(wildcard *.c)))

ifeq ($(shell uname), Darwin)
	LEAKTEST ?= leaks --atExit --
else
	LEAKTEST ?= valgrind --leak-check=full
endif

.PHONY: all valgrind clean test

all: shell tokenize

valgrind: shell tokenize 
	$(LEAKTEST) ./tokenize
	$(LEAKTEST) ./shell

format: 
	clang-format -style=Google -i *.c

tokenize-tests shell-tests : %-tests: %
	env python3 tests/$*_tests.py

test: tokenize-tests shell-tests 

clean: 
	rm -rf *.o
	rm -f shell tokenize

shell: $(SHELL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

tokenize: $(TOKENIZE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

