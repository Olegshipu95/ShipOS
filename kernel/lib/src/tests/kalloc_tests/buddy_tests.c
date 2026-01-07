#include "../../../include/test.h"
#include "../../../include/logging.h"
#include "../../../../kalloc/kalloc.h"
#include "alloc_common.h"

static void* buddy_alloc_wrapper(size_t sz) {
    return kmalloc(sz);
}

static void buddy_free_wrapper(void* p) {
    kmfree(p);
}

static struct allocator buddy_allocator = {
    .alloc = buddy_alloc_wrapper,
    .free  = buddy_free_wrapper
};

static int test_buddy_basic(void) { return test_alloc_basic(&buddy_allocator);}
static int test_buddy_zero_size(void) { return test_alloc_zero_size(&buddy_allocator);}
static int test_buddy_alignment(void) { return test_alloc_alignment(&buddy_allocator);}
static int test_buddy_reuse(void) {return test_alloc_reuse(&buddy_allocator);}
static int test_buddy_large_fails(void) { return test_alloc_large_fails(&buddy_allocator);}

static int test_buddy_split_merge(void) {
    void *a = kmalloc(128);
    void *b = kmalloc(128);
    if (!a || !b) return 0;

    kmfree(a);
    kmfree(b);

    void *c = kmalloc(256);
    if (!c) return 0;

    kmfree(c);
    return 1;
}

void run_buddy_tests(void) {
    LOG("Running Buddy allocator tests");

    TEST_REPORT("Buddy basic alloc/free", CHECK(test_buddy_basic));
    TEST_REPORT("Buddy zero-size", CHECK(test_buddy_zero_size));
    TEST_REPORT("Buddy alignment", CHECK(test_buddy_alignment));
    TEST_REPORT("Buddy reuse", CHECK(test_buddy_reuse));
    TEST_REPORT("Buddy large request fails", CHECK(test_buddy_large_fails));
    TEST_REPORT("Buddy split/merge", CHECK(test_buddy_split_merge));
}