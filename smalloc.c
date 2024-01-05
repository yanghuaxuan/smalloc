#include "smalloc.h"

#include <sys/mman.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>


#define MMAP_FLAGS PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
#define MMAP(n) mmap(NULL, n, MMAP_FLAGS)

#define PAGE 4096
#define PAGE_ALIGNMENT (PAGE - 1)
#define PAGE_ALIGN(N) (size_t)((N + PAGE_ALIGNMENT) & ~(PAGE_ALIGNMENT))

/* change this if size_t is not the same size as your architecture word size */
#define WORD (sizeof(size_t))

#define WORD_ALIGNMENT (WORD - 1)
#define WORD_ALIGN(N) (size_t)((N + WORD_ALIGNMENT) & ~(WORD_ALIGNMENT))

#define PAGEMAGIC 0xABCD1234

#define PRINTPAGESTATS \
    printf("===== Start of PRINTPAGESTATS =====\n"); \
    for (page_t* p = head; p != NULL; p = p->fd) { \
	printf("--- PAGE: SZ - %lu\n", p->size); \
    } \
    printf("===== End of PRINTPAGESTATS =====\n"); \

page_t* head = NULL;
page_t* tail = NULL;

static inline void page_remove(page_t* node) {
    if (node->bk)
	node->bk->fd = node->fd;
    if (node->fd)
	node->fd->bk = node->bk;

    if ((uintptr_t)tail == (uintptr_t)node) {
	tail = node->bk;
    }
    if ((uintptr_t)head == (uintptr_t)node) {
	head = node->fd;
    }
}

static inline void page_insert(page_t* node) {
    bool inserted = false;

    if (head == NULL && tail == NULL) {
	head = node;
	tail = node;
	return;
    }

    for (page_t* p = tail; p != NULL; p = p->bk) {
	if ((uintptr_t)node == (uintptr_t)p)
	    /* node is already in the list, so do nothing. */
	    return;
	if (p->size <= node->size) {
	    if (p->fd != NULL) {
		node->fd = p->fd;
		p->fd->bk = node;
	    } else {
		tail = node;
	    }
	    node->bk = p;
	    p->fd = node;

	    inserted = true;
	}
    }
    /* if still not inserted then this new node is the smallest node in the list */
    if (!inserted) {
	node->fd = head;
	if (head != NULL) {
	    head->bk = node;
	    head = node;
	}
    }
}

/* always allocates PAGE_ALIGN(n) bytes of memory */
static inline bool morecore(size_t n) {
    n = PAGE_ALIGN(n);
    void* m = MMAP(n);

    if (m == MAP_FAILED) {
	return false;
    }
    
    page_t* node = (page_t *)m;
    node->size = n - sizeof(page_t);
    node->fd = NULL;
    node->bk = NULL;
    node->magic = PAGEMAGIC;
    node->inuse = false;

    page_insert(node);

    return true;
}

void* smalloc(size_t n) {

    size_t na = (n + sizeof(page_t)); /* allocate enough memory for request + overhead */
    printf("::: Memory to allocate: %lu\n", na);
    for (page_t* p = head; p != NULL; p = p->fd) {
	assert(p->magic == PAGEMAGIC); /* this should never be false */
	if (p->size >= na) { 
	    p->size -= na; /* Give tail end of page to caller */
	    page_t* new = (page_t *)((char *)p + p->size);
	    new->magic = PAGEMAGIC;
	    new->size = na;
	    new->inuse = true;
	    PRINTPAGESTATS
	    return (void*)((char *)new + sizeof(page_t));
	}
    }
    /* try one more time */
    if (morecore(na)) {
	for (page_t* p = head; p != NULL; p = p->fd) {
	    assert(p->magic == PAGEMAGIC); /* this should never be false */
	    if (p->size >= na) {
		p->size -= na; /* Give tail end of page to caller */
	    	page_t* new = (page_t *)((char *)p + p->size);
		new->magic = PAGEMAGIC;
		new->size = na;
		new->inuse = true;
	    	PRINTPAGESTATS
	    	return (void*)((char *)new + sizeof(page_t));
	   }
        }
    }

    return NULL;
}

void sfree(void* ptr) {
    if (ptr == NULL)
	return;

    page_t* chk = (page_t *)((char *)ptr - sizeof(page_t));
    assert(chk->inuse == true);
    assert(chk->magic == PAGEMAGIC);
    chk->inuse = false;

    /* try joining upper contigious chunk */
    page_t* chk_upper = (page_t *)((char *)chk + chk->size);
    if (chk_upper->magic == PAGEMAGIC && chk_upper->inuse == false) {
	page_remove(chk_upper);
	printf("A (right) MAGIC?!?!?!\n");
	chk_upper->magic = 0x0;
	chk->size += chk_upper->size;
    }
    /* try joining lower contigious chunk */
    page_t* chk_lower = (page_t *)((char *)chk - chk->size);
    if (chk_lower->magic == PAGEMAGIC && chk_lower->inuse == false) {
	printf("A (left) MAGIC?!?!?!\n");
	chk_lower->magic = 0x0;
	chk_lower->size += chk->size;
	chk = chk_lower;
    }
//    if (chk_lower->magic == PAGEMAGIC && chk_lower->inuse == false) {
//	printf("A MAGIC AGAIN?!?!?!\n");
//	chk_lower->size += chk->size;
//	chk = chk_lower;
//    }

    page_insert(chk);
    //fprintf(stderr, "Not implemented yet!\n");
    PRINTPAGESTATS 
}

