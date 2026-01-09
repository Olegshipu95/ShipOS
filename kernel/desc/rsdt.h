//
// Created by ShipOS developers on 09.01.26.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//
// Utility file to work with Root System Description Table (RSDT)
//

#ifndef SHIP_OS_RSDT_H
#define SHIP_OS_RSDT_H

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include "rsdp.h"

struct ACPISDTHeader
{
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
};

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
