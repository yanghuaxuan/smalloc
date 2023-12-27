#include <unistd.h>

#include "smalloc.h"

#define NALLOC 1024

static Header base = {0};       /* list of free blocks */
static Header *freep = NULL;    /* pointer to last free block searched*/

/* Request block of memory */
static Header *morecore(size_t nu) {
    if (nu < NALLOC)
        nu = NALLOC;

    void *cp = sbrk(nu * sizeof(Header));
    if (cp == (void *) -1)
        return NULL;
    Header *up = (Header *)cp;
    up->s.size = nu;
    sfree((void *)(up + 1)); /* put it into the list of free blocks */

    return freep;
}

void *smalloc(size_t n) {
    /* round up to the proper amount of header-sized units */
    size_t nunits = ((n + sizeof(Header) - 1) / sizeof(Header)) + 1;
    Header *prevp = freep;

    if (freep == NULL) { /* no free list yet */
        base.s.next = freep = prevp = &base;
        base.s.size = 0;
    }

    /* find big enough page */
    for (Header *p = prevp->s.next; ;prevp = p, p = p->s.next) {
        if(p->s.size >= nunits) {   /* big enough page found */
            if (p->s.size == nunits)    /* exact size */
            prevp->s.next = p->s.next;
            else {              /* allocate tail end */
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }

            freep = prevp;
            return (void *)(p+1);
        }
        if (p == freep) { /* wrapped back. Need to expand heap */
            p = morecore(nunits);
            if (p == NULL)
                return NULL;
        }
    }
}

void sfree(void *ap) {
    Header *bp = (Header *)ap - 1; /* pointer to block header */

    Header *p = freep;
    /**
     * iterate until we find two blocks, such that
     * the address of bp is between the addresses
     * of the two blocks.
    **/
    for (; !((bp > p) && (bp < p->s.next)); p = p->s.next)
        if (p >= p->s.next && (bp > p || bp < p->s.next))
            break; /* freed block at start or end of list */

    if (bp + bp->s.size == p->s.next) { /* join to upper nbr */
        bp->s.size += p->s.next->s.size;
        bp->s.next = p->s.next->s.next;
    } else
        bp->s.next = p->s.next;
    if (p + p->s.size == bp) {          /* join to lower nbr */
        p->s.size += bp->s.size;
        p->s.next = bp->s.next;
    } else
        p->s.next = bp;

    freep = p;
}

