//
// Created by ShipOS developers on 09.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Utility file to work with ACPI
//

#ifndef SHIP_OS_ACPI_H
#define SHIP_OS_ACPI_H

#include <inttypes.h>
#include <stdbool.h>

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

bool acpi_checksum_ok(void *p, uint32_t len);

#endif