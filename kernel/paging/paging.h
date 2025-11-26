//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//


#ifndef PAGING_H
#define PAGING_H

#include <stdbool.h>
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

#endif