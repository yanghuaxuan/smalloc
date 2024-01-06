#ifndef SMALLOC_H
#define SMALLOC_H

#include <stddef.h>
#include <stdbool.h>

typedef struct chunk {
    size_t 		size;
    unsigned long 	magic;
    bool		inuse;
    struct chunk*	fd;
    struct chunk*	bk;
} chunk_t;

typedef struct foot {
    unsigned long 	magic;
    chunk_t*		head;
} foot_t;

void* smalloc(size_t n);
void sfree(void* ptr);

#endif
