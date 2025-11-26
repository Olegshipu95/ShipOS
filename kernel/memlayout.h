//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Memory layout definitions for ShipOS kernel
// Defines kernel start/end addresses, physical memory limits, and page properties.
//

#ifndef MEMLAYOUT_H
#define MEMLAYOUT_H

/**
 * @brief Kernel virtual memory start address.
 *
 * The kernel is linked to start at 1 MB (0x100000) in physical memory.
 */
#define KSTART 0x100000

/**
 * @brief Symbol representing the end of the kernel in memory.
 *
 * Defined in the linker script (linker.ld). Represents the first address
 * after the kernel's loaded code and data.
 */
extern char end[];
#define KEND end

/**
 * @brief Initial physical memory limit for page table allocation.
 *
 * Used when initializing the kernel's page tables.
 */
#define INIT_PHYSTOP (2 * 1024 * 1024) // 2 MB

/**
 * @brief Top of usable physical memory.
 *
 * The kernel can manage memory up to this address.
 */
#define PHYSTOP (128 * 1024 * 1024) // 128 MB

/**
 * @brief Size of one memory page in bytes.
 */
#define PGSIZE 4096 // 4 KB

/**
 * @brief Number of bits for the page offset.
 */
#define PGSHIFT 12 // log2(PGSIZE)

/**
 * @brief Round up a size to the nearest page boundary.
 *
 * Example: PGROUNDUP(5000) -> 8192
 */
#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))

/**
 * @brief Round down an address to the nearest page boundary.
 *
 * Example: PGROUNDDOWN(5000) -> 4096
 */
#define PGROUNDDOWN(a) ((a) & ~(PGSIZE - 1))

#endif // MEMLAYOUT_H
