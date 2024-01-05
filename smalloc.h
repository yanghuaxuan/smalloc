#ifndef SMALLOC_H
#define SMALLOC_H

#include <stddef.h>


typedef struct page {
    size_t 		size;
    unsigned long 	magic;
    struct page*	fd;
    struct page*	bk;
} page_t;

void* smalloc(size_t n);
void sfree(void* ptr);

#endif
