#include "slob.h"
#include "../kalloc.h"
#include "../../memlayout.h"

static slob_page_t *page_list = NULL;

static size_t align(size_t size) {
    return (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
}

static slob_page_t *slob_new_page() {
    void *page = kalloc();
    if (!page) return NULL;

    slob_page_t *p = (slob_page_t*)page;
    p->next = page_list;
    page_list = p;

    p->blocks = (slob_block_t*)(p + 1);
    p->blocks->next = NULL;
    p->blocks->size = PGSIZE - sizeof(slob_page_t);
    p->blocks->free = 1;

    return p;
}

void *slob_alloc(size_t size) {
    if (size > PGSIZE) {
        return NULL;
    }
    
    size = align(size);

    slob_page_t *page = page_list;
    while (page) {
        slob_block_t *block = page->blocks;
        while (block) {
            if (block->free && block->size >= size) {
                if (block->size >= size + sizeof(slob_block_t) + 8) {
                    slob_block_t *new_block = (slob_block_t*)((char*)block + sizeof(slob_block_t) + size);
                    new_block->size = block->size - size - sizeof(slob_block_t);
                    new_block->free = 1;
                    new_block->next = block->next;

                    block->size = size;
                    block->next = new_block;
                }

                block->free = 0;
                return (char*)block + sizeof(slob_block_t);
            }
            block = block->next;
        }
        page = page->next;
    }

    page = slob_new_page();
    if (!page) return NULL;
    return slob_alloc(size);
}

void slob_free(void *ptr) {
    if (!ptr) return;

    slob_block_t *block = (slob_block_t*)((char*)ptr - sizeof(slob_block_t));
    block->free = 1;

    slob_block_t *b = page_list->blocks;
    while (b) {
        if (b->free && b->next && b->next->free) {
            b->size += sizeof(slob_block_t) + b->next->size;
            b->next = b->next->next;
        } else {
            b = b->next;
        }
    }
}

size_t slob_get_page_count(void) {
    size_t cnt = 0;
    slob_page_t *p = page_list;
    while (p) {
        cnt++;
        p = p->next;
    }
    return cnt;
}

size_t slob_get_total_free_blocks(void) {
    size_t cnt = 0;
    slob_page_t *page = page_list;
    while (page) {
        slob_block_t *b = page->blocks;
        while (b) {
            if (b->free) cnt++;
            b = b->next;
        }
        page = page->next;
    }
    return cnt;
}

size_t slob_get_total_allocated_blocks(void) {
    size_t cnt = 0;
    slob_page_t *page = page_list;
    while (page) {
        slob_block_t *b = page->blocks;
        while (b) {
            if (!b->free) cnt++;
            b = b->next;
        }
        page = page->next;
    }
    return cnt;
}

int slob_has_adjacent_free_blocks(void) {
    slob_page_t *page = page_list;
    while (page) {
        slob_block_t *b = page->blocks;
        while (b && b->next) {
            if (b->free && b->next->free) {
                return 1;
            }
            b = b->next;
        }
        page = page->next;
    }
    return 0;
}
