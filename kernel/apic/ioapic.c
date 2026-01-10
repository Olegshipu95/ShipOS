//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// I/O APIC driver implementation
//

#include "ioapic.h"
#include "../desc/madt.h"
#include "../lib/include/logging.h"
#include <stddef.h>

#define MAX_IOAPICS 8

// I/O APIC register access offsets (in bytes)
#define IOAPIC_REGSEL 0x00 // Register Select (index)
#define IOAPIC_REGWIN 0x10 // Register Window (data)

static struct IOAPICInfo ioapics[MAX_IOAPICS];
static uint32_t ioapic_count = 0;

/**
 * @brief Write to an I/O APIC register
 */
static void ioapic_write(volatile uint32_t *ioapic_base, uint8_t reg, uint32_t value)
{
    volatile uint8_t *base = (volatile uint8_t *) ioapic_base;
    *((volatile uint32_t *) (base + IOAPIC_REGSEL)) = reg;
    *((volatile uint32_t *) (base + IOAPIC_REGWIN)) = value;
}

/**
 * @brief Read from an I/O APIC register
 */
static uint32_t ioapic_read(volatile uint32_t *ioapic_base, uint8_t reg)
{
    volatile uint8_t *base = (volatile uint8_t *) ioapic_base;
    *((volatile uint32_t *) (base + IOAPIC_REGSEL)) = reg;
    return *((volatile uint32_t *) (base + IOAPIC_REGWIN));
}

/**
 * @brief Find which I/O APIC handles a given GSI
 */
static struct IOAPICInfo *ioapic_for_gsi(uint32_t gsi)
{
    for (uint32_t i = 0; i < ioapic_count; i++)
    {
        uint32_t gsi_end = ioapics[i].gsi_base + ioapics[i].max_redirect + 1;
        if (gsi >= ioapics[i].gsi_base && gsi < gsi_end)
        {
            return &ioapics[i];
        }
    }
    return NULL;
}

/**
 * @brief Check if I/O APIC is available
 */
bool ioapic_is_available(void)
{
    return ioapic_count > 0;
}

/**
 * @brief Get the number of I/O APICs
 */
uint32_t ioapic_get_count(void)
{
    return ioapic_count;
}

/**
 * @brief Initialize all I/O APICs from MADT
 */
void ioapic_init(void)
{
    struct MADT_t *madt = get_madt();
    if (madt == NULL)
    {
        LOG_SERIAL("IOAPIC", "ERROR: No MADT table available");
        return;
    }

    // Parse MADT entries to find I/O APICs
    uint8_t *entry_ptr = (uint8_t *) madt + sizeof(struct MADT_t);
    uint8_t *end_ptr = (uint8_t *) madt + madt->header.Length;

    ioapic_count = 0;

    while (entry_ptr < end_ptr && ioapic_count < MAX_IOAPICS)
    {
        struct MADTEntryHeader *header = (struct MADTEntryHeader *) entry_ptr;

        if (header->Type == MADT_ENTRY_IOAPIC)
        {
            struct MADTEntryIOAPIC *entry = (struct MADTEntryIOAPIC *) entry_ptr;

            // Store I/O APIC information
            ioapics[ioapic_count].id = entry->IOAPICID;
            ioapics[ioapic_count].address = entry->IOAPICAddr;
            ioapics[ioapic_count].gsi_base = entry->GSIBase;

            // Map the I/O APIC registers (identity mapping assumed for now)
            ioapics[ioapic_count].regs = (volatile uint32_t *) (uintptr_t) entry->IOAPICAddr;

            // Read the version register to get max redirection entries
            uint32_t ver = ioapic_read(ioapics[ioapic_count].regs, IOAPIC_REG_VER);
            uint8_t max_entries = (ver >> 16) & 0xFF;

            // Set I/O APIC entries
            ioapics[ioapic_count].max_redirect = max_entries;

            LOG_SERIAL("IOAPIC", "Found I/O APIC %d at 0x%x, GSI base %d, max entries %d",
                       ioapics[ioapic_count].id,
                       ioapics[ioapic_count].address,
                       ioapics[ioapic_count].gsi_base,
                       ioapics[ioapic_count].max_redirect);

            ioapic_count++;
        }

        entry_ptr += header->Length;
    }

    if (ioapic_count == 0)
    {
        LOG_SERIAL("IOAPIC", "WARNING: No I/O APICs found");
        return;
    }

    // Mask all interrupts initially
    for (uint32_t i = 0; i < ioapic_count; i++)
    {
        for (uint8_t irq = 0; irq <= ioapics[i].max_redirect; irq++)
        {
            uint32_t gsi = ioapics[i].gsi_base + irq;
            ioapic_disable_irq(gsi);
        }
    }

    LOG_SERIAL("IOAPIC", "Initialized %d I/O APIC(s)", ioapic_count);
}

/**
 * @brief Set a redirection entry in the I/O APIC
 */
void ioapic_set_entry(uint8_t irq, uint8_t vector, uint8_t dest, uint32_t flags)
{
    struct IOAPICInfo *ioapic = ioapic_for_gsi(irq);
    if (ioapic == NULL)
    {
        LOG_SERIAL("IOAPIC", "ERROR: No I/O APIC for GSI %d", irq);
        return;
    }

    uint8_t idx = irq - ioapic->gsi_base;
    uint32_t low = vector | flags;
    uint32_t high = ((uint32_t) dest) << 24;

    // Write to the redirection table entry (each entry is 64 bits: low 32, high 32)
    ioapic_write(ioapic->regs, IOAPIC_REG_TABLE + 2 * idx, low);
    ioapic_write(ioapic->regs, IOAPIC_REG_TABLE + 2 * idx + 1, high);
}

/**
 * @brief Enable a specific IRQ on the I/O APIC
 */
void ioapic_enable_irq(uint8_t irq, uint8_t vector, uint8_t dest)
{
    // Default flags: Fixed delivery, physical destination, active high, edge triggered, not masked
    uint32_t flags = IOAPIC_DELMOD_FIXED | IOAPIC_DESTMOD_PHYSICAL |
                     IOAPIC_INTPOL_HIGH | IOAPIC_TRIGGER_EDGE;

    ioapic_set_entry(irq, vector, dest, flags);
}

/**
 * @brief Disable a specific IRQ on the I/O APIC
 */
void ioapic_disable_irq(uint8_t irq)
{
    struct IOAPICInfo *ioapic = ioapic_for_gsi(irq);
    if (ioapic == NULL)
    {
        return;
    }

    uint8_t idx = irq - ioapic->gsi_base;

    // Read current entry and set the mask bit
    uint32_t low = ioapic_read(ioapic->regs, IOAPIC_REG_TABLE + 2 * idx);
    low |= IOAPIC_MASKED;
    ioapic_write(ioapic->regs, IOAPIC_REG_TABLE + 2 * idx, low);
}
