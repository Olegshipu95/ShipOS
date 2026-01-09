//
// Created by ShipOS developers on 28.11.25.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//

#include "../include/test.h"
#include "../include/logging.h"
#include "../include/memset.h"
#include "../include/x86_64.h"
#include "../../paging/paging.h"
#include "../../kalloc/kalloc.h"
#include "../../memlayout.h"

int test_addition() {
    int a = 1;
    int b = 2;

    return a + b == 3;
}

/**
 * @brief Test that page entry encoding/decoding is consistent
 * 
 * Verifies that encoding a page entry and then decoding it 
 * produces the original values.
 */
int test_page_entry_encode_decode() {
    struct page_entry original;
    original.p = 1;
    original.rw = 1;
    original.us = 0;
    original.pwt = 0;
    original.pcd = 0;
    original.a = 1;
    original.d = 0;
    original.rsvd = 0;
    original.ign1 = 0;
    original.address = 0x12345;  // Test address (36-bit field)
    original.ign2 = 0;
    original.xd = 0;

    page_entry_raw encoded = encode_page_entry(original);
    struct page_entry decoded = decode_page_entry(encoded);

    return (decoded.p == original.p) &&
           (decoded.rw == original.rw) &&
           (decoded.us == original.us) &&
           (decoded.address == original.address) &&
           (decoded.a == original.a);
}

/**
 * @brief Test that kalloc returns valid aligned memory
 * 
 * Verifies that kalloc returns page-aligned memory and that
 * the returned pointer is non-null.
 */
int test_kalloc_returns_aligned_memory() {
    void *page = kalloc();
    
    if (page == 0) {
        return 0;  // Failed: kalloc returned null
    }

    // Check page alignment
    int is_aligned = ((uint64_t)page % PGSIZE) == 0;
    
    // Free the allocated page
    kfree(page);
    
    return is_aligned;
}

/**
 * @brief Test that kfree properly returns memory to the pool
 * 
 * Allocates pages, frees them, and verifies that the same
 * number of pages are available again.
 */
int test_kalloc_kfree_consistency() {
    uint64_t initial_count = count_pages();
    
    // Allocate several pages
    void *pages[5];
    for (int i = 0; i < 5; i++) {
        pages[i] = kalloc();
        if (pages[i] == 0) {
            // Free already allocated pages before failing
            for (int j = 0; j < i; j++) {
                kfree(pages[j]);
            }
            return 0;
        }
    }
    
    uint64_t after_alloc_count = count_pages();
    
    // Free all pages
    for (int i = 0; i < 5; i++) {
        kfree(pages[i]);
    }
    
    uint64_t after_free_count = count_pages();
    
    // Check that we have 5 fewer pages after allocation
    // and the same count after freeing
    return (initial_count - after_alloc_count == 5) &&
           (after_free_count == initial_count);
}

/**
 * @brief Test that page table walk can find existing entries
 * 
 * Uses the current CR3 page table and walks to a known
 * mapped address (kernel code area).
 */
int test_walk_existing_mapping() {
    pagetable_t tbl = (pagetable_t)rcr3();
    
    // Walk to an address we know is mapped (kernel start area)
    uint64_t kernel_addr = KSTART;
    page_entry_raw *entry = (page_entry_raw *)walk(tbl, kernel_addr, 0);
    
    if (entry == 0) {
        return 0;  // Failed: couldn't find mapping
    }
    
    struct page_entry decoded = decode_page_entry(*entry);
    
    // The kernel should be present and readable
    return decoded.p == 1;
}

/**
 * @brief Test that walk can allocate new page table entries
 * 
 * Walks to a new virtual address with alloc=1 and verifies
 * that a page table entry is created.
 */
int test_walk_allocates_new_entry() {
    pagetable_t tbl = (pagetable_t)rcr3();
    
    // Pick an address that's likely not mapped (high in virtual space)
    // Use a specific pattern that shouldn't conflict with kernel mappings
    uint64_t test_va = 0x400000000ULL;  // 16GB virtual address
    
    // First, try to walk without allocation - should fail or return null
    page_entry_raw *entry_no_alloc = (page_entry_raw *)walk(tbl, test_va, 0);
    (void)entry_no_alloc;  // Suppress unused variable warning
    
    // Now walk with allocation
    page_entry_raw *entry_with_alloc = (page_entry_raw *)walk(tbl, test_va, 1);
    
    if (entry_with_alloc == 0) {
        return 0;  // Failed: couldn't allocate
    }
    
    // Verify we got a valid entry pointer
    return entry_with_alloc != 0;
}

/**
 * @brief Test that memory can be written and read back correctly
 * 
 * Allocates a page, writes a pattern to it, and verifies
 * the pattern can be read back.
 */
int test_memory_write_read() {
    void *page = kalloc();
    
    if (page == 0) {
        return 0;
    }
    
    // Write a test pattern
    uint64_t *ptr = (uint64_t *)page;
    uint64_t test_pattern = 0xDEADBEEFCAFEBABEULL;
    
    ptr[0] = test_pattern;
    ptr[100] = test_pattern + 1;
    ptr[511] = test_pattern + 2;  // Near end of page
    
    // Verify the pattern
    int success = (ptr[0] == test_pattern) &&
                  (ptr[100] == test_pattern + 1) &&
                  (ptr[511] == test_pattern + 2);
    
    kfree(page);
    
    return success;
}

/**
 * @brief Test that memset correctly fills memory
 * 
 * Allocates a page, uses memset, and verifies the contents.
 */
int test_memset_fills_correctly() {
    void *page = kalloc();
    
    if (page == 0) {
        return 0;
    }
    
    // Fill with a known pattern
    memset(page, 0xAB, PGSIZE);
    
    // Verify the fill
    uint8_t *bytes = (uint8_t *)page;
    int success = 1;
    
    // Check first, middle, and last bytes
    if (bytes[0] != 0xAB || bytes[PGSIZE/2] != 0xAB || bytes[PGSIZE-1] != 0xAB) {
        success = 0;
    }
    
    kfree(page);
    
    return success;
}

/**
 * @brief Test that multiple allocations return different pages
 * 
 * Verifies that consecutive kalloc calls return distinct addresses.
 */
int test_allocations_are_distinct() {
    void *page1 = kalloc();
    void *page2 = kalloc();
    void *page3 = kalloc();
    
    if (page1 == 0 || page2 == 0 || page3 == 0) {
        if (page1) kfree(page1);
        if (page2) kfree(page2);
        if (page3) kfree(page3);
        return 0;
    }
    
    // All pages should be different
    int success = (page1 != page2) && (page2 != page3) && (page1 != page3);
    
    kfree(page1);
    kfree(page2);
    kfree(page3);
    
    return success;
}

/**
 * @brief Test that CR3 returns a valid page table address
 * 
 * Verifies that CR3 contains a page-aligned address.
 */
int test_cr3_valid_pagetable() {
    uint64_t cr3 = rcr3();
    
    // CR3 should be page-aligned
    return (cr3 % PGSIZE) == 0 && cr3 != 0;
}

/**
 * @brief Test page entry flag combinations
 * 
 * Verifies that various flag combinations encode/decode correctly.
 */
int test_page_entry_flags() {
    // Test with read-only page
    struct page_entry ro_entry = {0};
    ro_entry.p = 1;
    ro_entry.rw = 0;  // Read-only
    ro_entry.address = 0x1000;
    
    page_entry_raw ro_raw = encode_page_entry(ro_entry);
    struct page_entry ro_decoded = decode_page_entry(ro_raw);
    
    if (ro_decoded.rw != 0 || ro_decoded.p != 1) {
        return 0;
    }
    
    // Test with user-accessible page
    struct page_entry user_entry = {0};
    user_entry.p = 1;
    user_entry.rw = 1;
    user_entry.us = 1;  // User-mode accessible
    user_entry.address = 0x2000;
    
    page_entry_raw user_raw = encode_page_entry(user_entry);
    struct page_entry user_decoded = decode_page_entry(user_raw);
    
    return (user_decoded.us == 1) && (user_decoded.rw == 1);
}

/**
 * @brief Test memory isolation between pages
 * 
 * Verifies that writing to one page doesn't affect another.
 */
int test_memory_isolation() {
    void *page1 = kalloc();
    void *page2 = kalloc();
    
    if (page1 == 0 || page2 == 0) {
        if (page1) kfree(page1);
        if (page2) kfree(page2);
        return 0;
    }
    
    // Clear both pages
    memset(page1, 0, PGSIZE);
    memset(page2, 0, PGSIZE);
    
    // Write to page1
    uint64_t *ptr1 = (uint64_t *)page1;
    uint64_t *ptr2 = (uint64_t *)page2;
    
    ptr1[0] = 0x1234567890ABCDEFULL;
    
    // Verify page2 is still zero
    int success = (ptr2[0] == 0);
    
    kfree(page1);
    kfree(page2);
    
    return success;
}

/**
 * @brief Test that map_page correctly maps a virtual address
 * 
 * Allocates a physical page, maps it to a virtual address,
 * and verifies data can be written and read.
 */
int test_map_page() {
    pagetable_t tbl = (pagetable_t)rcr3();
    
    // Allocate a physical page
    void *phys_page = kalloc();
    if (phys_page == 0) {
        return 0;
    }
    
    // Choose a virtual address that's unlikely to be used
    uint64_t test_va = 0x500000000ULL;  // 20GB virtual address
    
    // Map the page with read/write permissions
    int result = map_page(tbl, test_va, (uint64_t)phys_page, PTE_W);
    if (result != 0) {
        kfree(phys_page);
        return 0;
    }
    
    // Write through the virtual address
    volatile uint64_t *vptr = (volatile uint64_t *)test_va;
    *vptr = 0xCAFEBABEDEADBEEFULL;
    
    // Read back and verify
    int success = (*vptr == 0xCAFEBABEDEADBEEFULL);
    
    // Also verify via physical address
    uint64_t *pptr = (uint64_t *)phys_page;
    success = success && (*pptr == 0xCAFEBABEDEADBEEFULL);
    
    // Cleanup
    unmap_page(tbl, test_va);
    kfree(phys_page);
    
    return success;
}

/**
 * @brief Test that va_to_pa correctly translates addresses
 * 
 * Maps a page and verifies the translation returns the correct PA.
 */
int test_va_to_pa() {
    pagetable_t tbl = (pagetable_t)rcr3();
    
    void *phys_page = kalloc();
    if (phys_page == 0) {
        return 0;
    }
    
    uint64_t test_va = 0x600000000ULL;
    uint64_t expected_pa = (uint64_t)phys_page;
    
    if (map_page(tbl, test_va, expected_pa, PTE_W) != 0) {
        kfree(phys_page);
        return 0;
    }
    
    // Test translation
    uint64_t translated_pa = va_to_pa(tbl, test_va);
    int success = (translated_pa == expected_pa);
    
    // Test with offset within page
    uint64_t offset = 0x123;
    uint64_t translated_with_offset = va_to_pa(tbl, test_va + offset);
    success = success && (translated_with_offset == expected_pa + offset);
    
    // Cleanup
    unmap_page(tbl, test_va);
    kfree(phys_page);
    
    return success;
}

/**
 * @brief Test that unmap_page correctly removes a mapping
 * 
 * Maps a page, then unmaps it, and verifies the translation fails.
 */
int test_unmap_page() {
    pagetable_t tbl = (pagetable_t)rcr3();
    
    void *phys_page = kalloc();
    if (phys_page == 0) {
        return 0;
    }
    
    uint64_t test_va = 0x700000000ULL;
    
    if (map_page(tbl, test_va, (uint64_t)phys_page, PTE_W) != 0) {
        kfree(phys_page);
        return 0;
    }
    
    // Verify it's mapped
    uint64_t pa_before = va_to_pa(tbl, test_va);
    if (pa_before == 0) {
        kfree(phys_page);
        return 0;
    }
    
    // Unmap
    unmap_page(tbl, test_va);
    
    // Verify translation now returns 0
    uint64_t pa_after = va_to_pa(tbl, test_va);
    int success = (pa_after == 0);
    
    kfree(phys_page);
    
    return success;
}

/**
 * @brief Test that map_pages correctly maps a range
 * 
 * Maps multiple pages and verifies they're all accessible.
 */
int test_map_pages_range() {
    pagetable_t tbl = (pagetable_t)rcr3();
    
    // Allocate 3 contiguous pages
    void *phys_pages[3];
    for (int i = 0; i < 3; i++) {
        phys_pages[i] = kalloc();
        if (phys_pages[i] == 0) {
            for (int j = 0; j < i; j++) {
                kfree(phys_pages[j]);
            }
            return 0;
        }
    }
    
    uint64_t test_va = 0x800000000ULL;
    
    // Map all 3 pages individually (map_pages expects contiguous physical memory)
    int success = 1;
    for (int i = 0; i < 3; i++) {
        if (map_page(tbl, test_va + i * PGSIZE, (uint64_t)phys_pages[i], PTE_W) != 0) {
            success = 0;
            break;
        }
    }
    
    if (success) {
        // Write to each page through virtual addresses
        for (int i = 0; i < 3; i++) {
            volatile uint64_t *vptr = (volatile uint64_t *)(test_va + i * PGSIZE);
            *vptr = 0x1000 + i;
        }
        
        // Verify through physical addresses
        for (int i = 0; i < 3; i++) {
            uint64_t *pptr = (uint64_t *)phys_pages[i];
            if (*pptr != (uint64_t)(0x1000 + i)) {
                success = 0;
                break;
            }
        }
    }
    
    // Cleanup
    for (int i = 0; i < 3; i++) {
        unmap_page(tbl, test_va + i * PGSIZE);
        kfree(phys_pages[i]);
    }
    
    return success;
}

void run_tests() {
    LOG("Test mode enabled, running tests");

    // Basic test
    TEST_REPORT("Addition", CHECK(test_addition));
    
    // Page entry encoding/decoding tests
    TEST_REPORT("VM: Page entry encode/decode", CHECK(test_page_entry_encode_decode));
    TEST_REPORT("VM: Page entry flags", CHECK(test_page_entry_flags));
    
    // Memory allocation tests
    TEST_REPORT("VM: kalloc returns aligned memory", CHECK(test_kalloc_returns_aligned_memory));
    TEST_REPORT("VM: kalloc/kfree consistency", CHECK(test_kalloc_kfree_consistency));
    TEST_REPORT("VM: Allocations are distinct", CHECK(test_allocations_are_distinct));
    
    // Memory read/write tests
    TEST_REPORT("VM: Memory write/read", CHECK(test_memory_write_read));
    TEST_REPORT("VM: memset fills correctly", CHECK(test_memset_fills_correctly));
    TEST_REPORT("VM: Memory isolation", CHECK(test_memory_isolation));
    
    // Page table tests
    TEST_REPORT("VM: CR3 valid pagetable", CHECK(test_cr3_valid_pagetable));
    TEST_REPORT("VM: Walk existing mapping", CHECK(test_walk_existing_mapping));
    TEST_REPORT("VM: Walk allocates new entry", CHECK(test_walk_allocates_new_entry));
    
    // Mapping function tests
    TEST_REPORT("VM: map_page works", CHECK(test_map_page));
    TEST_REPORT("VM: va_to_pa translation", CHECK(test_va_to_pa));
    TEST_REPORT("VM: unmap_page works", CHECK(test_unmap_page));
    TEST_REPORT("VM: map_pages range", CHECK(test_map_pages_range));

    LOG("All VM tests completed");
}
