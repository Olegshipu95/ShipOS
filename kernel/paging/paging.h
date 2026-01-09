//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//


#ifndef PAGING_H
#define PAGING_H

#include "stdbool.h"
//#include "../lib/include/stdint.h"
#include <inttypes.h>

#define ENTRIES_COUNT 512

typedef uint64_t page_entry_raw;

typedef page_entry_raw* pagetable_t;

struct page_entry {
    // Present; must be 1 to map a page
    bool p;

    // Read/write; if 0, writes may not be allowed to the page referenced by this entry
    bool rw;

    // User/supervisor; if 0, user-mode accesses are not allowed to the page referenced by this entry
    bool us;

    // Page-level write-through; indirectly determines the memory type used to access the page referenced by this entry
    bool pwt;

    // Page-level cache disable; indirectly determines the memory type used to access the page referenced by this entry
    bool pcd;

    // Accessed; indicates whether software has accessed the  page referenced by this entry
    bool a;

    // Dirty; indicates whether software has written to the page referenced by this entry
    bool d;

    // Indirectly determines the memory type used to access the page referenced by this entry
    bool rsvd; // 1 if points to page, 0 if to the another table

    uint8_t ign1; // 4 bits

    // Physical address of the page referenced by this entry
    uintptr_t address; // 36 bits

    uint32_t ign2; // 15 bits ?

    // Exec-disable
    bool xd; // 😆😆😆
};

struct page_entry_t {
    page_entry_raw table[ENTRIES_COUNT];
};

page_entry_raw encode_page_entry(struct page_entry);

struct page_entry decode_page_entry(page_entry_raw);

void init_paging();

void print_vm(pagetable_t);

pagetable_t kvminit(uint64_t, uint64_t);

struct page_entry_raw *walk(pagetable_t tbl, uint64_t va, bool alloc);

/**
 * @brief Map a single page from virtual address to physical address
 * 
 * @param tbl Page table to use (typically from rcr3())
 * @param va Virtual address to map (will be page-aligned)
 * @param pa Physical address to map to (will be page-aligned)
 * @param flags Page flags (PTE_W for writable, PTE_U for user-accessible)
 * @return 0 on success, -1 on failure
 */
int map_page(pagetable_t tbl, uint64_t va, uint64_t pa, int flags);

/**
 * @brief Map a range of physical memory into virtual address space
 * 
 * @param tbl Page table to use
 * @param va Virtual address start (will be page-aligned)
 * @param pa Physical address start (will be page-aligned)
 * @param size Size in bytes to map
 * @param flags Page flags
 * @return 0 on success, -1 on failure
 */
int map_pages(pagetable_t tbl, uint64_t va, uint64_t pa, uint64_t size, int flags);

/**
 * @brief Map physical memory for MMIO/device access (identity mapping)
 * 
 * Maps physical address to the same virtual address.
 * Useful for accessing ACPI tables, device memory, etc.
 * 
 * @param pa Physical address to map
 * @param size Size in bytes to map
 * @return Virtual address (same as pa) on success, 0 on failure
 */
void *map_mmio(uint64_t pa, uint64_t size);

/**
 * @brief Unmap a single page
 * 
 * @param tbl Page table to use
 * @param va Virtual address to unmap
 */
void unmap_page(pagetable_t tbl, uint64_t va);

/**
 * @brief Unmap a range of pages
 * 
 * @param tbl Page table to use
 * @param va Virtual address start
 * @param size Size in bytes to unmap
 */
void unmap_pages(pagetable_t tbl, uint64_t va, uint64_t size);

/**
 * @brief Translate virtual address to physical address
 * 
 * @param tbl Page table to use
 * @param va Virtual address to translate
 * @return Physical address, or 0 if not mapped
 */
uint64_t va_to_pa(pagetable_t tbl, uint64_t va);

/**
 * @brief Invalidate TLB entry for a virtual address
 * 
 * @param va Virtual address to invalidate
 */
void invlpg(uint64_t va);

// Page table entry flags
#define PTE_P   0x001   // Present
#define PTE_W   0x002   // Writable
#define PTE_U   0x004   // User-accessible
#define PTE_PWT 0x008   // Page-level write-through
#define PTE_PCD 0x010   // Page-level cache disable
#define PTE_A   0x020   // Accessed
#define PTE_D   0x040   // Dirty

#endif