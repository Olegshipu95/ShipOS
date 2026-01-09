//
// Created by ShipOS developers on 09.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Utility file to work with Root System Description Table (RSDT)
//

#ifndef SHIP_OS_RSDT_H
#define SHIP_OS_RSDT_H

#include "acpi.h"
#include "rsdp.h"

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

struct RSDT_t
{
    struct ACPISDTHeader header;
    uint32_t PointerToOtherSDT[];
} __attribute__((packed));

struct XSDT_t
{
    struct ACPISDTHeader header;
    uint64_t PointerToOtherSDT[];
} __attribute__((packed));

void init_rsdt(struct RSDP_t *rsdp_ptr);

void *get_rsdt_root();

bool is_xsdt();

/**
 * @brief Find an ACPI table by signature in RSDT/XSDT
 * 
 * @param signature 4-character signature (e.g., "APIC", "FACP")
 * @return Pointer to the table header, or NULL if not found
 */
struct ACPISDTHeader *rsdt_find_table(const char *signature);

/**
 * @brief Get the number of entries in the RSDT/XSDT
 * 
 * @return Number of table entries
 */
uint32_t rsdt_get_entry_count();

#endif
