CC := gcc
CFLAGS := -Wall -pedantic  -g -ggdb

test: test.c smalloc.c
	$(CC) $(CFLAGS) $^ -o test

clean:
	rm -f test

.PHONY: clean
