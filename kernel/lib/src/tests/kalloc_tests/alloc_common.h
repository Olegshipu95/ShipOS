#ifndef ALLOC_COMMON_H
#define ALLOC_COMMON_H

#include <stddef.h>

struct allocator {
    void* (*alloc)(size_t size);
    void (*free)(void* ptr);
};

int test_alloc_basic(struct allocator *a);
int test_alloc_null_free(struct allocator *a);
int test_alloc_zero_size(struct allocator *a);
int test_alloc_alignment(struct allocator *a);
int test_alloc_reuse(struct allocator *a);
int test_alloc_large_fails(struct allocator *a);

#endif