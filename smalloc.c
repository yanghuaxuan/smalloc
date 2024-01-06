#include "smalloc.h"

#include <sys/mman.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/* portable(?) anonymous mmapping */
static void* __mmap(size_t n) {
    int fd = open("/dev/zero", O_RDWR);
    if (fd == -1)
	return NULL;
    void* m = mmap(NULL, n, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);
    if (m == MAP_FAILED)
	return NULL;

    return m;
}

#define MMAP(n) __mmap(n)

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
    for (chunk_t* p = head; p != NULL; p = p->fd) { \
	printf("--- PAGE: SZ - %lu\n", p->size); \
    } \
    printf("===== End of PRINTPAGESTATS =====\n"); \

chunk_t* head = NULL;
chunk_t* tail = NULL;

static inline void page_remove(chunk_t* node) {
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

static inline void page_insert(chunk_t* node) {
    bool inserted = false;

    if (head == NULL && tail == NULL) {
	head = node;
	tail = node;
	return;
    }

    for (chunk_t* p = tail; p != NULL; p = p->bk) {
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
    
    chunk_t* node = (chunk_t *)m;
    node->size = n - sizeof(chunk_t) - sizeof(foot_t);
    node->fd = NULL;
    node->bk = NULL;
    node->magic = PAGEMAGIC;
    node->inuse = false;

    foot_t* foot = (foot_t *)((char *)node + sizeof(chunk_t) + node->size);
    foot->magic = PAGEMAGIC;
    foot->head = node;

    page_insert(node);

    return true;
}

/* search for the first block that is >= size and return that block */
static inline chunk_t* page_search(size_t size) {
    for (chunk_t* p = head; p != NULL; p = p->fd) {
	assert(p->magic == PAGEMAGIC); /* this should never be false */
	if (p->size >= size) { 
	    return p;
	}
    }
    return NULL;
}

/* TODO: Move foot everytiem */
void* smalloc(size_t n) {

    size_t na = (sizeof(chunk_t) + n + sizeof(foot_t)); /* allocate enough memory for request + overhead */
    printf("::: Memory to allocate: %lu\n", na);
    chunk_t* chk = page_search(na);
    if (chk == NULL) {
	if (morecore(na)) {
	    /* try one more time */
	    chk = page_search(na);
	} else {
	    return NULL;
	}
    }

    chk->size += sizeof(foot_t); 
    chk->size -= na ; /* give tail end of page to caller */

    chunk_t* new = (chunk_t *)((char *)chk + chk->size);
    new->magic = PAGEMAGIC;
    new->size = n;
    new->inuse = true;
    new->fd = NULL;
    new->bk = NULL;

    foot_t* new_foot = (foot_t *)((char *)new + sizeof(chunk_t ) + new->size );
    new_foot->magic = PAGEMAGIC;
    new_foot->head = new;

    chk->size -= sizeof(foot_t);
    foot_t* chk_foot = (foot_t *)((char *)chk + chk->size);
    chk_foot->magic = PAGEMAGIC;
    chk_foot->head = chk;

    return (void *)((char *)new + sizeof(chunk_t));
}

/* TODO: Cleanup code */
void sfree(void* ptr) {
    if (ptr == NULL)
	return;

    bool insert_page = true;

    chunk_t* chk = (chunk_t *)((char *)ptr - sizeof(chunk_t));
    assert(chk->magic == PAGEMAGIC); /* invalid chunk pointer */
    chk->inuse = false;

    foot_t* chk_footer = (foot_t *)((char *)chk + sizeof(chunk_t) + chk->size);
    assert(chk_footer->magic == PAGEMAGIC);

    /* try joining lower contigious chunk */
    foot_t* chk_lower = (foot_t *)((char *)chk - sizeof(foot_t));
    if (chk_lower->magic == PAGEMAGIC && chk_lower->head->inuse == false) {
	printf("A (left) MAGIC?!?!?!\n");
	chk_lower->head->size += sizeof(chunk_t) + chk->size + sizeof(foot_t);
	chk_footer->magic = PAGEMAGIC;
	chk_footer->head = chk_lower->head;
	insert_page = false;

	chk = chk_footer->head;

    }
    /* try joining upper contigious chunk */
    chunk_t* chk_upper = (chunk_t *)((char *)chk_footer + sizeof(foot_t));
    if (chk_upper->magic == PAGEMAGIC && chk_upper->inuse == false) {
	page_remove(chk_upper);
	printf("A (right) MAGIC?!?!?!\n");
	chk->size += sizeof(chunk_t) + chk_upper->size + sizeof(foot_t);
    }

    if (insert_page)
	page_insert(chk);
    PRINTPAGESTATS 
}

