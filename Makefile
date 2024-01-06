CC := gcc
CFLAGS := -Wall -pedantic  -std=c99  -O2

test: test.c smalloc.c
	$(CC) $(CFLAGS) $^ -o test

clean:
	rm -f test

.PHONY: clean
