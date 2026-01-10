//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// I/O APIC (Advanced Programmable Interrupt Controller) driver for x86_64
//

#ifndef SHIP_OS_IOAPIC_H
#define SHIP_OS_IOAPIC_H

#include <inttypes.h>
#include <stdbool.h>

// I/O APIC register selectors
#define IOAPIC_REG_ID 0x00    // I/O APIC ID
#define IOAPIC_REG_VER 0x01   // I/O APIC Version
#define IOAPIC_REG_ARB 0x02   // I/O APIC Arbitration ID
#define IOAPIC_REG_TABLE 0x10 // Redirection Table base (entries 0-23)

// Redirection table entry bits
#define IOAPIC_DELMOD_FIXED 0x00000000  // Delivery Mode: Fixed
#define IOAPIC_DELMOD_LOWEST 0x00000100 // Delivery Mode: Lowest Priority
#define IOAPIC_DELMOD_SMI 0x00000200    // Delivery Mode: SMI
#define IOAPIC_DELMOD_NMI 0x00000400    // Delivery Mode: NMI
#define IOAPIC_DELMOD_INIT 0x00000500   // Delivery Mode: INIT
#define IOAPIC_DELMOD_EXTINT 0x00000700 // Delivery Mode: ExtINT

#define IOAPIC_DESTMOD_PHYSICAL 0x00000000 // Destination Mode: Physical
#define IOAPIC_DESTMOD_LOGICAL 0x00000800  // Destination Mode: Logical

#define IOAPIC_DELIVS 0x00001000        // Delivery Status (read-only)
#define IOAPIC_INTPOL_HIGH 0x00000000   // Interrupt Polarity: Active High
#define IOAPIC_INTPOL_LOW 0x00002000    // Interrupt Polarity: Active Low
#define IOAPIC_REMOTEIRR 0x00004000     // Remote IRR (read-only)
#define IOAPIC_TRIGGER_EDGE 0x00000000  // Trigger Mode: Edge
#define IOAPIC_TRIGGER_LEVEL 0x00008000 // Trigger Mode: Level
#define IOAPIC_MASKED 0x00010000        // Interrupt Mask: Masked

// Default interrupt vectors for I/O APIC
#define IOAPIC_IRQ_BASE 32 // Base vector for I/O APIC interrupts

/**
 * @brief I/O APIC information structure
 */
struct IOAPICInfo
{
    uint8_t id;              // I/O APIC ID
    uint32_t address;        // Physical address
    uint32_t gsi_base;       // Global System Interrupt base
    uint8_t max_redirect;    // Maximum redirection entry
    volatile uint32_t *regs; // Virtual address of registers
};

/**
 * @brief Initialize all I/O APICs found in the system
 *
 * Discovers I/O APICs from MADT and initializes them.
 * Sets up redirection entries for standard IRQs.
 */
void ioapic_init(void);

/**
 * @brief Enable a specific IRQ on the I/O APIC
 *
 * @param irq IRQ number (0-23)
 * @param vector Interrupt vector to map to
 * @param dest Destination APIC ID
 */
void ioapic_enable_irq(uint8_t irq, uint8_t vector, uint8_t dest);

/**
 * @brief Disable a specific IRQ on the I/O APIC
 *
 * @param irq IRQ number (0-23)
 */
void ioapic_disable_irq(uint8_t irq);

/**
 * @brief Set a redirection entry in the I/O APIC
 *
 * @param irq IRQ number
 * @param vector Interrupt vector
 * @param dest Destination APIC ID
 * @param flags Additional flags (delivery mode, polarity, trigger mode, etc.)
 */
void ioapic_set_entry(uint8_t irq, uint8_t vector, uint8_t dest, uint32_t flags);

/**
 * @brief Get the number of I/O APICs in the system
 *
 * @return uint32_t Number of I/O APICs
 */
uint32_t ioapic_get_count(void);

/**
 * @brief Check if I/O APIC is available
 *
 * @return bool True if at least one I/O APIC is available
 */
bool ioapic_is_available(void);

#endif // SHIP_OS_IOAPIC_H
