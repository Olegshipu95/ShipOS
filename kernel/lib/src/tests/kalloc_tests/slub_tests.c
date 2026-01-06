#include "../../../include/test.h"
#include "../../../include/logging.h"
#include "../../../../kalloc/slub/slub.h"
#include "alloc_common.h"

static void* slub_alloc_wrapper(size_t sz) { return malloc_slub(sz); }
static void slub_free_wrapper(void* p) { free_slub(p); }

static struct allocator slub_allocator = {
    .alloc = slub_alloc_wrapper,
    .free = slub_free_wrapper
};

static int test_slub_basic(void) { return test_alloc_basic(&slub_allocator); }
static int test_slub_null_free(void) { return test_alloc_null_free(&slub_allocator); }
static int test_slub_zero_size(void) { return test_alloc_zero_size(&slub_allocator); }
static int test_slub_alignment(void) { return test_alloc_alignment(&slub_allocator); }
static int test_slub_reuse(void) { return test_alloc_reuse(&slub_allocator); }
static int test_slub_large_fails(void) { return test_alloc_large_fails(&slub_allocator); }

void run_slub_tests(void) {
    LOG("Running SLUB allocator tests");

    TEST_REPORT("SLUB basic alloc/free", CHECK(test_slub_basic));
    TEST_REPORT("SLUB free(NULL) safe", CHECK(test_slub_null_free));
    TEST_REPORT("SLUB zero-size", CHECK(test_slub_zero_size));
    TEST_REPORT("SLUB alignment", CHECK(test_slub_alignment));
    TEST_REPORT("SLUB reuse", CHECK(test_slub_reuse));
    TEST_REPORT("SLUB large request fails", CHECK(test_slub_large_fails));
}