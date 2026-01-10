//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//


#include "paging.h"
#include "../tty/tty.h"
#include "../kalloc/kalloc.h"
#include "../lib/include/memset.h"
#include "../memlayout.h"
#include "../lib/include/x86_64.h"
#include "../lib/include/logging.h"

page_entry_raw encode_page_entry(struct page_entry entry) {

    page_entry_raw raw = 0;

    raw |= (entry.p & 0x1);
    raw |= (entry.rw & 0x1) << 1;
    raw |= (entry.us & 0x1) << 2;
    raw |= (entry.pwt & 0x1) << 3;
    raw |= (entry.pcd & 0x1) << 4;
    raw |= (entry.a & 0x1) << 5;
    raw |= (entry.d & 0x1) << 6;
    raw |= (entry.rsvd & 0x1) << 7;
    raw |= (entry.ign1 & 0xF) << 8;
    raw |= (entry.address & 0xFFFFFFFFF) << 12;
    raw |= (entry.ign2 & 0x7FFF) << 48;
    raw |= (entry.xd & 0x1) << 63;

    return raw;

}

struct page_entry decode_page_entry(page_entry_raw raw) {
    struct page_entry entry;

    entry.p = raw & 0x1;
    entry.rw = (raw >> 1) & 0x1;
    entry.us = (raw >> 2) & 0x1;
    entry.pwt = (raw >> 3) & 0x1;
    entry.pcd = (raw >> 4) & 0x1;
    entry.a = (raw >> 5) & 0x1;
    entry.d = (raw >> 6) & 0x1;
    entry.rsvd = (raw >> 7) & 0x1;
    entry.ign1 = (raw >> 8) & 0xF;
    entry.address = (raw >> 12) & 0xFFFFFFFFF;
    entry.ign2 = (raw >> 48) & 0x7FFF;
    entry.xd = (raw >> 63) & 0x1;

    return entry;
}

void print_entry(struct page_entry *entry) {
    printf("P: %d RW: %d US: %d PWT: %d A: %d D: %d ADDR: %p\n", entry->p, entry->rw, entry->us, entry->pwt, entry->a, entry->d, entry->address << 12);
}

void do_print_vm(pagetable_t tbl, int level) {
    int spaces = 4 - level + 1;
    for (size_t i = 0; i < 512; i++) {
        struct page_entry entry = decode_page_entry(tbl[i]);
        if (entry.p) {
            for (int j = 0; j < spaces; j++) {
                print(".. ");
            }
            print_entry(&entry);
            if (level > 1) do_print_vm(entry.address << 12, level-1);
        }
    }
}

void print_vm(pagetable_t tbl) {
    do_print_vm(tbl, 4);
}


// Pass in address to raw entry to initialize
// and the full address (will be cut to 36 bits inside the function)
void init_entry(page_entry_raw *raw_entry, uint64_t addr) {
    struct page_entry entry;

    entry.p = 1;    
    entry.rw = 1;
    entry.us = 0;
    entry.pwt = 0;
    entry.pcd = 0;
    entry.a = 0;
    entry.d = 0;
    entry.rsvd = 0;
    entry.ign1 = 0;
    entry.address = (addr >> 12) & 0xFFFFFFFFF;
    entry.ign2 = 0;
    entry.xd = 0;

    *raw_entry = encode_page_entry(entry);
}

struct page_entry_raw *walk(pagetable_t tbl, uint64_t va, bool alloc) {
    for (int level = 3; level > 0; level--) {
        int level_index = (va >> (12 + level * 9)) & 0x1FF;
        // printf("VA: %p TBL: %p LEVEL: %d INDEX: %d\n", va, tbl, level, level_index);
        page_entry_raw *entry_raw = &tbl[level_index];
        struct page_entry entry = decode_page_entry(*entry_raw);
        // print_entry(&entry);

        if (entry.p) {
            // printf("PRESENT, CONTINUE\n");
            tbl = entry.address << 12;
        } else {
            // printf("NOT PRESENT, ALLOC NEW TABLE\n");
            if (alloc == 0 || (tbl = kalloc()) == 0) {
                return 0;
            }
            // printf("Allocated page at %p\n", tbl);
            memset(tbl, 0, PGSIZE);
            init_entry(entry_raw, (uint64_t)tbl);
        }
    }

    // printf("ENTRY IN TABLE %p\n", tbl);
    return tbl + ((va >> 12) & 0x1FF);
}

/**
 * @brief Map APIC memory regions into kernel page table
 * 
 * Maps Local APIC and I/O APIC addresses for MMIO access.
 * APIC addresses are typically in high memory (~4GB range).
 */
void map_apic_region(pagetable_t tbl, uint64_t apic_base, uint32_t size) {
    LOG("Mapping APIC region at 0x%x (size: %d bytes)", apic_base, size);
    
    // Map each page in the APIC region
    for (uint64_t addr = apic_base; addr < apic_base + size; addr += PGSIZE) {
        page_entry_raw *entry_raw = walk(tbl, addr, 1);
        if (entry_raw == 0) {
            LOG("ERROR: Failed to walk page table for APIC at 0x%x", addr);
            continue;
        }
        
        // Initialize entry with identity mapping (virtual = physical)
        // Mark as uncacheable (PCD=1) for MMIO regions
        struct page_entry entry;
        entry.p = 1;     // Present
        entry.rw = 1;    // Read/Write
        entry.us = 0;    // Supervisor only
        entry.pwt = 0;   // Write-through
        entry.pcd = 1;   // Cache disable (important for MMIO!)
        entry.a = 0;     // Accessed
        entry.d = 0;     // Dirty
        entry.rsvd = 0;  // Not reserved
        entry.ign1 = 0;
        entry.address = (addr >> 12) & 0xFFFFFFFFF;
        entry.ign2 = 0;
        entry.xd = 0;    // Execute disable
        
        *entry_raw = encode_page_entry(entry);
    }
    
    // Flush TLB for the mapped region
    for (uint64_t addr = apic_base; addr < apic_base + size; addr += PGSIZE) {
        invlpg(addr);
    }
}

pagetable_t kvminit(uint64_t start, uint64_t end) {
    LOG("Setting up kernel page table...");
    pagetable_t tbl4 = rcr3();

    char *addr;
    addr = (char*)PGROUNDUP(start);
    for(;addr + PGSIZE < end; addr += PGSIZE) {
        //printf("Walking address %p\n", addr);
        page_entry_raw *entry_raw = walk(tbl4, addr, 1);
        // printf("FOUND ENTRY AT ADDRESS: %p\n", entry_raw);
        // TODO an infinite busy-wait loop. Add panic
        while (entry_raw == 0) {}
        init_entry(entry_raw, addr);
        // printf("INITIALIZED ENTRY: %p\n", addr);
    }

    return tbl4;
}

void invlpg(uint64_t va) {
    asm volatile("invlpg (%0)" : : "r"(va) : "memory");
}

int map_page(pagetable_t tbl, uint64_t va, uint64_t pa, int flags) {
    va = PGROUNDDOWN(va);
    pa = PGROUNDDOWN(pa);
    
    page_entry_raw *pte = (page_entry_raw *)walk(tbl, va, 1);
    if (pte == 0) {
        return -1;
    }
    
    struct page_entry entry = decode_page_entry(*pte);
    
    if (entry.p && (entry.address << 12) != pa) {
        LOG("map_page: va %p already mapped to %p, trying to map to %p",
            va, entry.address << 12, pa);
        return -1;
    }
    
    entry.p = 1;
    entry.rw = (flags & PTE_W) ? 1 : 0;
    entry.us = (flags & PTE_U) ? 1 : 0;
    entry.pwt = (flags & PTE_PWT) ? 1 : 0;
    entry.pcd = (flags & PTE_PCD) ? 1 : 0;
    entry.a = 0;
    entry.d = 0;
    entry.rsvd = 0;
    entry.ign1 = 0;
    entry.address = (pa >> 12) & 0xFFFFFFFFF;
    entry.ign2 = 0;
    entry.xd = 0;
    
    *pte = encode_page_entry(entry);
    
    invlpg(va);
    
    return 0;
}

int map_pages(pagetable_t tbl, uint64_t va, uint64_t pa, uint64_t size, int flags) {
    uint64_t va_start = PGROUNDDOWN(va);
    uint64_t va_end = PGROUNDUP(va + size);
    uint64_t pa_cur = PGROUNDDOWN(pa);
    
    for (uint64_t addr = va_start; addr < va_end; addr += PGSIZE) {
        if (map_page(tbl, addr, pa_cur, flags) != 0) {
            for (uint64_t rollback = va_start; rollback < addr; rollback += PGSIZE) {
                unmap_page(tbl, rollback);
            }
            return -1;
        }
        pa_cur += PGSIZE;
    }
    
    return 0;
}

void *map_mmio(uint64_t pa, uint64_t size) {
    pagetable_t tbl = (pagetable_t)rcr3();
    
    uint64_t pa_aligned = PGROUNDDOWN(pa);
    uint64_t offset = pa - pa_aligned;
    uint64_t map_size = size + offset;
    
    if (map_pages(tbl, pa_aligned, pa_aligned, map_size, PTE_W | PTE_PCD) != 0) {
        return 0;
    }
    
    return (void *)(uintptr_t)pa;
}

void unmap_page(pagetable_t tbl, uint64_t va) {
    va = PGROUNDDOWN(va);
    
    page_entry_raw *pte = (page_entry_raw *)walk(tbl, va, 0);
    if (pte == 0) {
        return;
    }
    
    struct page_entry entry = decode_page_entry(*pte);
    if (!entry.p) {
        return;
    }
    
    *pte = 0;
    
    invlpg(va);
}

void unmap_pages(pagetable_t tbl, uint64_t va, uint64_t size) {
    uint64_t va_start = PGROUNDDOWN(va);
    uint64_t va_end = PGROUNDUP(va + size);
    
    for (uint64_t addr = va_start; addr < va_end; addr += PGSIZE) {
        unmap_page(tbl, addr);
    }
}

uint64_t va_to_pa(pagetable_t tbl, uint64_t va) {
    page_entry_raw *pte = (page_entry_raw *)walk(tbl, va, 0);
    if (pte == 0) {
        return 0;
    }
    
    struct page_entry entry = decode_page_entry(*pte);
    if (!entry.p) {
        return 0;
    }
    
    return (entry.address << 12) | (va & (PGSIZE - 1));
}

// Legacy hack
// pagetable_t kvminit(uint64_t start, uint64_t end){

//     // pagetable_t tbl4 = (pagetable_t) kalloc();
//     // memset(tbl4,0,4096);
//     // pagetable_t tbl3 = (pagetable_t) kalloc();
//     // memset(tbl3,0,4096);
//     // pagetable_t tbl2 = (pagetable_t) kalloc();
//     // memset(tbl2,0,4096);
//     // pagetable_t tbl1 = (pagetable_t) kalloc();
//     // memset(tbl1,0,4096);
//     pagetable_t tbl4 = rcr3();
//     pagetable_t tbl3 = decode_page_entry(tbl4[0]).address << 12;
//     pagetable_t tbl2 = decode_page_entry(tbl3[0]).address << 12;


//     for (int j = 1; j < 512; ++j)
//     {
//         pagetable_t tbl1 = kalloc();
//         init_entry(tbl2 + j, (uint64_t) tbl1);

//         for (uint64_t i = 0; i < 512; ++i){
//             init_entry(tbl1 + i, start + (i << 12) + ((j - 1) << 9 << 12) );
//         }
//     }


//     return tbl4;
// }
