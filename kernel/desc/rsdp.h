//
// Created by ShipOS developers on 09.01.26.
// Copyright (c) 2025 SHIPOS. All rights reserved.
//
// Utility file to work with Root System Description Pointer (RSDP)
//

#ifndef SHIP_OS_RSDP_H
#define SHIP_OS_RSDP_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

struct RSDP_t
{
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__((packed));

struct XSDP_t
{
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;

    // ACPI 2.0+
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__((packed));

void init_rsdp();

void *get_rsdp();

bool is_xsdp();

#endif
