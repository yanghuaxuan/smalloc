#ifndef SMALLOC
#define SMALLOC

#include <stddef.h>
#include <stdbool.h>

typedef long Align;

typedef union Header {
  struct {
    union Header *next;
    size_t size;
  } s;
  Align _x;
} Header;

/* Allocate n bytes of memory to the heap. 
 *  Probably singlethreaded only
 *  Memory is not initialized
*/
void *smalloc(size_t n);

void sfree(void *ptr);

#endif
