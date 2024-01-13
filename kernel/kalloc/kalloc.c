#include "../kalloc/kalloc.h"
#include "../sync/spinlock.h"
#include "../memlayout.h"
//#include "../lib/include/stdint.h"
#include <inttypes.h>
#include <stddef.h>
#include "../tty/tty.h"

struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
} kmem;


void kinit(uint64_t start, uint64_t stop) {
    //  initlock(&kmem.lock, "kmem");
    char *p;
    p = (char *) PGROUNDUP(start);
    for (; p + PGSIZE < stop; p += PGSIZE)
        kfree(p);
}

void kfree(void *pa) {
    struct run *r;

    if (((uint64_t) pa % PGSIZE) != 0 || (char *) pa < end || (uint64_t) pa >= PHYSTOP) {
        printf("Panic while trying to free memory\nPA: %p END: %p PHYSTOP: %p", pa, end, PHYSTOP);
        panic("kfree");
    }

    // Fill with junk to catch dangling refs.
    char *testpa = (pa + PGSIZE - 1);
    memset(pa, 0, PGSIZE);

    r = (struct run *) pa;

//    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
//    release(&kmem.lock);
}

void *kalloc() {
    struct run *r;

//    acquire(&kmem.lock);
    r = kmem.freelist;
    if (r)
        kmem.freelist = r->next;
//    release(&kmem.lock);

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

    return res;
}