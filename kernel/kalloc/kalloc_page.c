#include "../kalloc/kalloc.h"
#include "../sync/spinlock.h"
#include "../memlayout.h"
//#include "../lib/include/stdint.h"
#include <inttypes.h>
#include <stddef.h>
#include "../lib/include/logging.h"

struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
} kmem;


void kinit(uint64_t start, uint64_t stop) {
    // TODO Race Cond.
    //  init_spinlock(&kmem.lock, "kmem");
    char *p;
    p = (char *) PGROUNDUP(start);
    for (; p + PGSIZE < stop; p += PGSIZE)
        kfree(p);
}

void kfree(void *pa) {
    // TODO Race Cond.
    struct run *r;

    if (((uint64_t) pa % PGSIZE) != 0 || (char *) pa < end || (uint64_t) pa >= PHYSTOP) {
        LOG("Panic while trying to free memory\nPA: %p END: %p PHYSTOP: %p", pa, end, PHYSTOP);
        panic("kfree");
    }

    // Fill with junk to catch dangling refs.
    char *testpa = (pa + PGSIZE - 1);
    memset(pa, 0, PGSIZE);

    r = (struct run *) pa;

//    acquire_spinlock(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
//    release_spinlock(&kmem.lock);
}

void *kalloc() {
    // TODO Race Cond.
    struct run *r;

//    acquire_spinlock(&kmem.lock);
    r = kmem.freelist;
    if (r)
        kmem.freelist = r->next;
//    release_spinlock(&kmem.lock);

    if (r)
        memset((char *) r, 5, PGSIZE); // fill with junk
    return (void *) r;
}

uint64_t count_pages() {
    struct run *r = kmem.freelist;
    uint64_t res = 0;

    while (r != 0) {
        res++;
        r = r->next;
    }

    LOG("%d pages available in allocator", res);
    return res;
}

void *kmalloc(size_t) {
    panic("kmalloc not implemented for page allocator");
}

void kmfree(void*) {
    panic("kmfree not implemented for page allocator");
}