#include "../../../include/test.h"
#include "../../../include/logging.h"
#include "../../../../kalloc/slab/slab.h"
#include "../../../../kalloc/slab/slab_test.h"
#include "alloc_common.h"
#include <string.h>

static void* slab_alloc_wrapper(size_t sz) { return kmalloc_slab(sz); }
static void slab_free_wrapper(void* p) { kfree_slab(p); }

static struct allocator slab_allocator = {
    .alloc = slab_alloc_wrapper,
    .free = slab_free_wrapper
};

static int test_slab_basic(void) {
    return test_alloc_basic(&slab_allocator);
}

static int test_slab_null_free(void) {
    return test_alloc_null_free(&slab_allocator);
}

static int test_slab_zero_size(void) {
    return test_alloc_zero_size(&slab_allocator);
}

static int test_slab_alignment(void) {
    return test_alloc_alignment(&slab_allocator);
}

static int test_slab_reuse(void) {
    return test_alloc_reuse(&slab_allocator);
}

static int test_slab_large_fails(void) {
    return test_alloc_large_fails(&slab_allocator);
}

static int test_slab_partial_tracking(void) {
    init_slab_cache();
    void *p = kmalloc_slab(8);
    if (!p) return 0;

    size_t partial = slab_get_cache_slabs_partial_count(0);
    size_t empty = slab_get_cache_slabs_empty_count(0);
    kfree_slab(p);
    size_t empty_after = slab_get_cache_slabs_empty_count(0);

    return (partial == 1 && empty == 0 && empty_after == 1);
}

void run_slab_tests(void) {
    LOG("Running slab allocator tests");

    TEST_REPORT("Slab basic alloc/free", CHECK(test_slab_basic));
    TEST_REPORT("Slab free(NULL) safe", CHECK(test_slab_null_free));
    TEST_REPORT("Slab zero-size", CHECK(test_slab_zero_size));
    TEST_REPORT("Slab alignment", CHECK(test_slab_alignment));
    TEST_REPORT("Slab reuse", CHECK(test_slab_reuse));
    TEST_REPORT("Slab large request fails", CHECK(test_slab_large_fails));
    TEST_REPORT("Slab partial slab tracking", CHECK(test_slab_partial_tracking));
}