CC := gcc
CFLAGS := -Wall -pedantic  -g -ggdb -fsanitize=address

test: test.c smalloc.c
	$(CC) $(CFLAGS) $^ -o test

clean:
	rm -f test

.PHONY: clean
