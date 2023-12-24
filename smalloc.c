#include "smalloc.h"

#include <stddef.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static Header base = {0};
static Header *freep = NULL;

#define NALLOC 1024

/* If testing, uncomment this, and comment morecore() */

// Limited size heap for testing smalloc
static char buf[(NALLOC) * sizeof(Header)] = {0};
char *buf_p = buf;

static Header *morecore(size_t nu) {
  if (nu < NALLOC)
    nu = NALLOC;

  if ((buf_p + nu * sizeof(Header)) > (buf + NALLOC * sizeof(Header)))
    return NULL;

  char *cp = buf_p;
  buf_p += (nu * sizeof(Header));

  Header *up = (Header *)cp;
  up->s.size = nu;
  sfree((void *)(up + 1)); /* Put it into the list of free blocks */
  return freep;
}

/* Request block of memory */
//static Header *morecore(size_t nu) {
//  if (nu < NALLOC)
//    nu = NALLOC;
//  char *cp = mmap(NULL, nu * sizeof(Header), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
//  if (cp == MAP_FAILED) 
//    return NULL;
//  Header *up = (Header *)cp;
//  up->s.size = nu;
//  sfree((void *)(up + 1)); /* Put it into the list of free blocks */
//  return freep;
//}

void *smalloc(size_t n) {
  Header *prevp;
  // Round up to the proper amount of header-sized units
  size_t nunits = ((n + sizeof(Header) - 1) / sizeof(Header)) + 1;

  if (freep == NULL) {
    base.s.next = freep = prevp = &base;
    base.s.size = 0;
  }

  // Find big enough page
  for (Header *p = prevp->s.next; ;prevp = p, p = p->s.next) {
     if(p->s.size >= nunits) { /* Big enough page found */
       if (p->s.size == nunits) /* Exact size */
         prevp->s.next = p->s.next;
       else { /* Allocate tail end */
         p->s.size -= nunits; 
         p += p->s.size;
         p->s.size = nunits;
       }

       freep = prevp;
       return (void *)(p+1);
    }
    if (p == freep) { /* Wrapped back. Need to expand heap */
      p = morecore(nunits);
      if (p == NULL) {
	return NULL;
      }
    }
  }
}

void sfree(void *ptr) {
  Header *bp, *p; 
  p = NULL;
  bp = (Header *)ptr - 1; /* Pointer to block header */

  /** Find a spot to insert back the block. This is either between two existing
   *  blocks, or one at the end of the list.
  */
  for (p = freep; bp <= p || bp >= p->s.next; p = p->s.next)
    if (p >= p->s.next && (bp > p || bp < p->s.next))
      break; /* Freed block at start or end of arena */

  if (bp + bp->s.size == p->s.next) { /* Join to upper nbr */
    bp->s.size += p->s.next->s.size;
    bp->s.next = p->s.next->s.next;
  } else
    bp->s.next = p->s.next;
  if (p + p->s.size == bp) { /* Join to lower nbr */
    p->s.size += bp->s.size;
    p->s.next = bp->s.next;
  } else
    p->s.next = bp;

  freep = p;
}
