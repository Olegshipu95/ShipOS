#include "../../../include/test.h"
#include "../../../include/logging.h"
#include "../../../../kalloc/slob/slob.h"
#include "../../../../kalloc/slob/slob_test.h"
#include "alloc_common.h"

static void* slob_alloc_wrapper(size_t sz) { return slob_alloc(sz); }
static void slob_free_wrapper(void* p) { slob_free(p); }

static struct allocator slob_allocator = {
    .alloc = slob_alloc_wrapper,
    .free = slob_free_wrapper
};

static int test_slob_basic(void) { return test_alloc_basic(&slob_allocator); }
static int test_slob_null_free(void) { return test_alloc_null_free(&slob_allocator); }
static int test_slob_zero_size(void) { return test_alloc_zero_size(&slob_allocator); }
static int test_slob_alignment(void) { return test_alloc_alignment(&slob_allocator); }
static int test_slob_reuse(void) { return test_alloc_reuse(&slob_allocator); }
static int test_slob_large_fails(void) { return test_alloc_large_fails(&slob_allocator); }

static int test_slob_coalescing(void) {
    if (slob_has_adjacent_free_blocks()) return 0;

    void *a = slob_alloc(64);
    void *b = slob_alloc(64);
    if (!a || !b) return 0;

    if (slob_has_adjacent_free_blocks()) {
        slob_free(a); slob_free(b);
        return 0;
    }

    slob_free(a);
    slob_free(b);

    if (slob_has_adjacent_free_blocks()) {
        return 0;
    }

    if (slob_get_total_free_blocks() != 1) {
        return 0;
    }

    return 1;
}

void run_slob_tests(void) {
    LOG("Running SLOB allocator tests");

    TEST_REPORT("SLOB basic alloc/free", CHECK(test_slob_basic));
    TEST_REPORT("SLOB free(NULL) safe", CHECK(test_slob_null_free));
    TEST_REPORT("SLOB zero-size", CHECK(test_slob_zero_size));
    TEST_REPORT("SLOB alignment", CHECK(test_slob_alignment));
    TEST_REPORT("SLOB reuse", CHECK(test_slob_reuse));
    TEST_REPORT("SLOB large request fails", CHECK(test_slob_large_fails));
    TEST_REPORT("SLOB coalescing sanity", CHECK(test_slob_coalescing));
}