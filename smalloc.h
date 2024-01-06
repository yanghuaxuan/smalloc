#ifndef SMALLOC_H
#define SMALLOC_H

#include <stddef.h>
#include <stdbool.h>

typedef struct page {
    size_t 		size;
    unsigned long 	magic;
    bool		inuse;
    struct page*	fd;
    struct page*	bk;
} page_t;

typedef struct foot {
    unsigned long 	magic;
    page_t*		head;
} foot_t;

void* smalloc(size_t n);
void sfree(void* ptr);

#endif
