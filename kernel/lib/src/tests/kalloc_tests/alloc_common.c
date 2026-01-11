#include "alloc_common.h"
#include "../../../include/test.h"

int test_alloc_basic(struct allocator *a) {
    void *p = a->alloc(64);
    if (!p) return -1;
    memset(p, 0xAB, 64);
    a->free(p);
    if (*(int*)p != 0) return -1;
    return 1;
}

int test_alloc_null_free(struct allocator *a) {
    a->free(NULL);
    return 1;
}

int test_alloc_zero_size(struct allocator *a) {
    void *p = a->alloc(0);
    if (p) a->free(p);
    return 1;
}

int test_alloc_alignment(struct allocator *a) {
    for (int i = 1; i <= 128; i++) {
        void *p = a->alloc(i);
        if (!p) return -1;
        if ((size_t) p % sizeof(void*) != 0) {
            a->free(p);
            return -1;
        }
        a->free(p);
    }
    return 1;
}

int test_alloc_reuse(struct allocator *a) {
    void *p1 = a->alloc(32);
    if (!p1) return 0;
    void *p2 = a->alloc(32);
    if (!p2) { a->free(p1); return -1; }
    a->free(p1);
    void *p3 = a->alloc(32);

    if (!p3) { a->free(p2); return -1; }
    if (p1 != p3) { a->free(p2); a->free(p3); return -1; }

    a->free(p2);
    a->free(p3);
    
    return 1;
}

int test_alloc_large_fails(struct allocator *a) {
    void *p = a->alloc(1024 * 1024);
    if (p) {
        a->free(p);
        return -1;
    }
    return 1;
}