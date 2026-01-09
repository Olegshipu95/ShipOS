//
// Created by ShipOS developers on 09.01.26.
// Copyright (c) 2025 SHIPOS. All rights reserved.
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
    uint32_t *PointerToOtherSDT;
};

struct XSDT_t
{
    struct ACPISDTHeader header;
    uint64_t *PointerToOtherSDT;
};

void init_rsdt(struct RSDP_t *rsdp_ptr);

void *get_rsdt_root();

bool is_xsdt();

#endif
