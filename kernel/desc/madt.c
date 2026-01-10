//
// Created by ShipOS developers on 09.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Utility file to work with Multiple APIC Description Table (MADT)
//

#include "madt.h"
#include "rsdt.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include "../lib/include/logging.h"
#include "../lib/include/memset.h"
#include "../kalloc/kalloc.h"

// Static storage for MADT and CPU info
static struct MADT_t *madt_ptr = NULL;
static uint32_t lapic_address = 0;
static struct CPUInfo cpus[MAX_CPUS];
static uint32_t cpu_count = 0;
static struct IOAPICEntry ioapics[MAX_IOAPICS];
static uint32_t ioapic_count = 0;

struct MADT_t *get_madt(void)
{
    return madt_ptr;
}

uint32_t get_lapic_address(void)
{
    return lapic_address;
}

uint32_t get_cpu_count(void)
{
    return cpu_count;
}

struct CPUInfo *get_cpu_info(uint32_t index)
{
    if (index >= cpu_count)
    {
        return NULL;
    }
    return &cpus[index];
}

uint32_t get_ioapic_count(void)
{
    return ioapic_count;
}

struct IOAPICEntry *get_ioapic_info(uint32_t index)
{
    if (index >= ioapic_count)
    {
        return NULL;
    }
    return &ioapics[index];
}

static void parse_madt_entries(struct MADT_t *madt)
{
    // Calculate where entries start and end
    uint8_t *entry_ptr = (uint8_t *) madt + sizeof(struct MADT_t);
    uint8_t *end_ptr = (uint8_t *) madt + madt->header.Length;

    while (entry_ptr < end_ptr)
    {
        struct MADTEntryHeader *entry = (struct MADTEntryHeader *) entry_ptr;

        // Sanity check
        if (entry->Length < 2 || entry_ptr + entry->Length > end_ptr)
        {
            LOG_SERIAL("MADT", "Invalid entry at offset %d (len=%d)",
                       (int) (entry_ptr - (uint8_t *) madt), entry->Length);
            break;
        }

        switch (entry->Type)
        {
        case MADT_ENTRY_LAPIC:
        {
            struct MADTEntryLAPIC *lapic = (struct MADTEntryLAPIC *) entry;

            if (cpu_count < MAX_CPUS)
            {
                cpus[cpu_count].acpi_id = lapic->ACPIProcessorID;
                cpus[cpu_count].apic_id = lapic->APICID;
                cpus[cpu_count].flags = lapic->Flags;
                cpus[cpu_count].enabled = (lapic->Flags & LAPIC_FLAG_ENABLED) != 0;
                cpus[cpu_count].is_bsp = (cpu_count == 0); // First CPU found is typically BSP

                cpu_count++;
            }
            break;
        }

        case MADT_ENTRY_IOAPIC:
        {
            struct MADTEntryIOAPIC *ioapic_entry = (struct MADTEntryIOAPIC *) entry;

            if (ioapic_count < MAX_IOAPICS)
            {
                ioapics[ioapic_count].id = ioapic_entry->IOAPICID;
                ioapics[ioapic_count].address = ioapic_entry->IOAPICAddr;
                ioapics[ioapic_count].gsi_base = ioapic_entry->GSIBase;
                ioapic_count++;
            }
            break;
        }

        case MADT_ENTRY_ISO:
            break;

        case MADT_ENTRY_LAPIC_NMI:
            break;

        case MADT_ENTRY_LAPIC_OVERRIDE:
        {
            struct MADTEntryLAPICOverride *override = (struct MADTEntryLAPICOverride *) entry;
            lapic_address = (uint32_t) override->LAPICAddr;
            break;
        }

        case MADT_ENTRY_X2APIC:
        {
            struct MADTEntryX2APIC *x2apic = (struct MADTEntryX2APIC *) entry;

            if (cpu_count < MAX_CPUS)
            {
                cpus[cpu_count].acpi_id = (uint8_t) x2apic->ACPIProcessorUID;
                cpus[cpu_count].apic_id = (uint8_t) x2apic->X2APICID;
                cpus[cpu_count].flags = x2apic->Flags;
                cpus[cpu_count].enabled = (x2apic->Flags & LAPIC_FLAG_ENABLED) != 0;
                cpus[cpu_count].is_bsp = (cpu_count == 0);

                cpu_count++;
            }
            break;
        }

        default:
            break;
        }

        entry_ptr += entry->Length;
    }
}

void init_madt(void)
{
    cpu_count = 0;
    ioapic_count = 0;
    madt_ptr = NULL;
    lapic_address = 0;

    // Find MADT table using RSDT lookup (signature is "APIC")
    struct ACPISDTHeader *madt_header = rsdt_find_table("APIC");

    if (madt_header == NULL)
    {
        LOG_SERIAL("MADT", "MADT table not found");
        return;
    }

    madt_ptr = (struct MADT_t *) madt_header;

    // Get Local APIC address from MADT header
    lapic_address = madt_ptr->LAPICAddr;

    // Parse all entries
    parse_madt_entries(madt_ptr);
}

void log_cpu_info(void)
{
    uint32_t enabled_count = 0;
    for (uint32_t i = 0; i < cpu_count; i++)
    {
        if (cpus[i].enabled)
        {
            enabled_count++;
        }
    }
    LOG_SERIAL("CPU", "Detected %d CPUs (%d enabled), LAPIC at 0x%x",
               cpu_count, enabled_count, lapic_address);
}

void madt_copy_to_safe_memory(void)
{
    if (madt_ptr == NULL)
    {
        return;
    }

    struct MADT_t *old_madt = madt_ptr;
    uint32_t table_size = old_madt->header.Length;

    // Allocate new memory from kalloc (safe region)
    void *new_madt = kalloc();
    if (new_madt == NULL)
    {
        LOG_SERIAL("MADT", "Failed to allocate memory for MADT copy");
        return;
    }

    // Copy the entire MADT table
    memset(new_madt, 0, 4096);
    for (uint32_t i = 0; i < table_size; i++)
    {
        ((uint8_t *) new_madt)[i] = ((uint8_t *) old_madt)[i];
    }

    madt_ptr = (struct MADT_t *) new_madt;
    LOG_SERIAL("MADT", "Copied to safe memory at %p (size=%d bytes)", new_madt, table_size);
}