//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Local APIC (Advanced Programmable Interrupt Controller) driver implementation
//

#include "lapic.h"
#include "../desc/madt.h"
#include "../lib/include/logging.h"
#include "../lib/include/x86_64.h"
#include <stddef.h>

// Virtual address where LAPIC registers are mapped
static volatile uint32_t *lapic = NULL;

/**
 * @brief Write to a Local APIC register
 */
void lapic_write(uint32_t reg, uint32_t value)
{
    if (lapic == NULL) return;
    lapic[reg >> 2] = value;
}

/**
 * @brief Read from a Local APIC register
 */
uint32_t lapic_read(uint32_t reg)
{
    if (lapic == NULL) return 0;
    return lapic[reg >> 2];
}

/**
 * @brief Check if Local APIC is available
 */
bool lapic_is_available(void)
{
    return lapic != NULL;
}

/**
 * @brief Get the ID of the current Local APIC
 */
uint8_t lapic_get_id(void)
{
    if (lapic == NULL) return 0;
    return (uint8_t)(lapic_read(LAPIC_ID) >> 24);
}

/**
 * @brief Initialize the Local APIC for the current CPU
 */
void lapic_init(void)
{
    // Get LAPIC base address from MADT
    uint32_t lapic_phys = get_lapic_address();
    
    if (lapic_phys == 0)
    {
        LOG_SERIAL("LAPIC", "ERROR: No LAPIC address found in MADT");
        return;
    }

    // For now, directly map the physical address (identity mapping assumed)
    // In a more complete system, you'd use proper virtual memory mapping
    lapic = (volatile uint32_t *)(uintptr_t)lapic_phys;

    LOG_SERIAL("LAPIC", "Physical address: 0x%x", lapic_phys);

    // Enable the APIC by setting bit 8 in the Spurious Interrupt Vector Register
    // Also set the spurious interrupt vector to 0xFF
    lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | LAPIC_SPURIOUS_VECTOR);

    // Clear Task Priority Register to allow all interrupts
    lapic_write(LAPIC_TPR, 0);

    // Set up the error interrupt vector
    lapic_write(LAPIC_ERROR, LAPIC_ERROR_VECTOR);

    // Clear error status register (write to clear)
    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_ESR, 0);

    // Acknowledge any outstanding interrupts
    lapic_write(LAPIC_EOI, 0);

    // Enable LINT0 and LINT1 as needed
    // Mask LINT0 and LINT1 for now (set bit 16)
    lapic_write(LAPIC_LINT0, LAPIC_TIMER_MASKED);
    lapic_write(LAPIC_LINT1, LAPIC_TIMER_MASKED);

    // Disable performance counter overflow interrupts
    if ((lapic_read(LAPIC_VERSION) >> 16) >= 4)
    {
        lapic_write(LAPIC_PERF, LAPIC_TIMER_MASKED);
    }

    // Map error interrupt
    lapic_write(LAPIC_ERROR, LAPIC_ERROR_VECTOR);

    // Clear error status register again
    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_ESR, 0);

    // Acknowledge any outstanding interrupts
    lapic_write(LAPIC_EOI, 0);

    LOG_SERIAL("LAPIC", "Initialized on CPU with APIC ID %d", lapic_get_id());
}

/**
 * @brief Send End-Of-Interrupt signal to Local APIC
 */
void lapic_eoi(void)
{
    if (lapic == NULL) return;
    lapic_write(LAPIC_EOI, 0);
}

/**
 * @brief Start the Local APIC timer in periodic mode
 */
void lapic_timer_start(uint8_t vector, uint32_t initial_count)
{
    if (lapic == NULL) return;

    // Set the divide configuration to divide by 1
    lapic_write(LAPIC_TIMER_DCR, LAPIC_TIMER_DIV_16);

    // Set the timer in periodic mode with the specified vector
    lapic_write(LAPIC_TIMER, LAPIC_TIMER_PERIODIC | vector);

    // Set the initial count to start the timer
    lapic_write(LAPIC_TIMER_ICR, initial_count);

    LOG_SERIAL("LAPIC", "Timer started with vector %d, initial count %d", vector, initial_count);
}

/**
 * @brief Stop the Local APIC timer
 */
void lapic_timer_stop(void)
{
    if (lapic == NULL) return;

    // Mask the timer interrupt
    lapic_write(LAPIC_TIMER, LAPIC_TIMER_MASKED);
    
    // Set initial count to 0 to stop the timer
    lapic_write(LAPIC_TIMER_ICR, 0);
}
