#ifndef SLOB_H
#define SLOB_H

#include <stddef.h>

typedef struct slob_block {
    struct slob_block *next;
    size_t size;
    int free;
} slob_block_t;

typedef struct slob_page {
    struct slob_page *next;
    slob_block_t *blocks;
} slob_page_t;

void *slob_alloc(size_t size);
void slob_free(void *ptr);

static slob_page_t* slob_new_page(void);

#endif