#include "../../../include/test.h"
#include "../../../include/logging.h"
#include "../../../../kalloc/kalloc.h"
#include "alloc_common.h"

static int test_page_basic(void) {
    void *p = kalloc();
    if (!p) return -1;

    memset(p, 0xAA, PGSIZE);
    kfree(p);
    if (!*(int*)p) return -1;
    return 1;
}

static int test_page_exhaustion(void) {
    uint64_t before = count_pages();

    void *pages[128];
    int i = 0;

    for (; i < 128; i++) {
        pages[i] = kalloc();
        if (!pages[i]) break;
    }

    if (i == 0) return 0;

    for (int j = 0; j < i; j++)
        kfree(pages[j]);

    uint64_t after = count_pages();
    return before == after;
}

void run_page_tests(void) {
    LOG("Running Page allocator tests");

    TEST_REPORT("Page basic alloc/free", CHECK(test_page_basic));
    TEST_REPORT("Page exhaustion/reuse", CHECK(test_page_exhaustion));
}
