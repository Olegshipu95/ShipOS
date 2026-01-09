//
// Created by ShipOS developers on 09.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Utility file to work with Multiple APIC Description Table (MADT)
//

#ifndef SHIP_OS_MADT_H
#define SHIP_OS_MADT_H

#include "acpi.h"
#include <stdbool.h>

// MADT Entry Types
#define MADT_ENTRY_LAPIC            0   // Processor Local APIC
#define MADT_ENTRY_IOAPIC           1   // I/O APIC
#define MADT_ENTRY_ISO              2   // Interrupt Source Override
#define MADT_ENTRY_NMI_SOURCE       3   // NMI Source
#define MADT_ENTRY_LAPIC_NMI        4   // Local APIC NMI
#define MADT_ENTRY_LAPIC_OVERRIDE   5   // Local APIC Address Override
#define MADT_ENTRY_X2APIC           9   // Processor Local x2APIC

// MADT Flags
#define MADT_FLAG_PCAT_COMPAT       (1 << 0)  // PC-AT compatible dual 8259 setup

// Local APIC flags
#define LAPIC_FLAG_ENABLED          (1 << 0)  // Processor is usable
#define LAPIC_FLAG_ONLINE_CAPABLE   (1 << 1)  // Processor can be enabled

#define MAX_CPUS 64

struct MADT_t
{
    struct ACPISDTHeader header;
    uint32_t LAPICAddr;     // Physical address of Local APIC
    uint32_t Flags;         // MADT flags
    // Variable length array of entries follows
} __attribute__((packed));

struct MADTEntryHeader
{
    uint8_t Type;
    uint8_t Length;
} __attribute__((packed));

// Type 0: Processor Local APIC
struct MADTEntryLAPIC
{
    struct MADTEntryHeader header;
    uint8_t ACPIProcessorID;    // ACPI Processor UID
    uint8_t APICID;             // Local APIC ID
    uint32_t Flags;             // LAPIC flags
} __attribute__((packed));

// Type 1: I/O APIC
struct MADTEntryIOAPIC
{
    struct MADTEntryHeader header;
    uint8_t IOAPICID;
    uint8_t Reserved;
    uint32_t IOAPICAddr;        // Physical address of I/O APIC
    uint32_t GSIBase;           // Global System Interrupt base
} __attribute__((packed));

// Type 2: Interrupt Source Override
struct MADTEntryISO
{
    struct MADTEntryHeader header;
    uint8_t BusSource;
    uint8_t IRQSource;
    uint32_t GSI;               // Global System Interrupt
    uint16_t Flags;
} __attribute__((packed));

// Type 4: Local APIC NMI
struct MADTEntryLAPICNMI
{
    struct MADTEntryHeader header;
    uint8_t ACPIProcessorID;    // 0xFF means all processors
    uint16_t Flags;
    uint8_t LINT;               // LINT# (0 or 1)
} __attribute__((packed));

// Type 5: Local APIC Address Override (64-bit)
struct MADTEntryLAPICOverride
{
    struct MADTEntryHeader header;
    uint16_t Reserved;
    uint64_t LAPICAddr;         // 64-bit physical address of Local APIC
} __attribute__((packed));

// Type 9: Processor Local x2APIC
struct MADTEntryX2APIC
{
    struct MADTEntryHeader header;
    uint16_t Reserved;
    uint32_t X2APICID;
    uint32_t Flags;
    uint32_t ACPIProcessorUID;
} __attribute__((packed));

// CPU information structure
struct CPUInfo
{
    uint8_t  apic_id;           // Local APIC ID
    uint8_t  acpi_id;           // ACPI Processor ID
    uint32_t flags;             // LAPIC flags
    bool     is_bsp;            // Is this the bootstrap processor?
    bool     enabled;           // Is this CPU enabled?
};

void init_madt(void);

struct MADT_t *get_madt(void);

uint32_t get_lapic_address(void);

uint32_t get_cpu_count(void);

struct CPUInfo *get_cpu_info(uint32_t index);

void log_cpu_info(void);

#endif
