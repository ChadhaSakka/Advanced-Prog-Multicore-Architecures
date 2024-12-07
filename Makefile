# Makefile

CC = gcc
CFLAGS = -O2 -Wall -pthread -std=c11

all: test_allocator

test_allocator: test-allocator.c lock-based-allocator.c lock-free-allocator.c
	$(CC) $(CFLAGS) -fsanitize=thread -g -o test-allocator test-allocator.c lock-based-allocator.c lock-free-allocator.c

clean:
	rm -f test-allocator

