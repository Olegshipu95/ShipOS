//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Local APIC (Advanced Programmable Interrupt Controller) driver for x86_64
//

#ifndef SHIP_OS_LAPIC_H
#define SHIP_OS_LAPIC_H

#include <inttypes.h>
#include <stdbool.h>

// Local APIC register offsets (divide by 4 for 32-bit array indexing)
#define LAPIC_ID                0x0020  // Local APIC ID Register
#define LAPIC_VERSION           0x0030  // Local APIC Version Register
#define LAPIC_TPR               0x0080  // Task Priority Register
#define LAPIC_APR               0x0090  // Arbitration Priority Register
#define LAPIC_PPR               0x00A0  // Processor Priority Register
#define LAPIC_EOI               0x00B0  // End of Interrupt Register
#define LAPIC_RRD               0x00C0  // Remote Read Register
#define LAPIC_LDR               0x00D0  // Logical Destination Register
#define LAPIC_DFR               0x00E0  // Destination Format Register
#define LAPIC_SVR               0x00F0  // Spurious Interrupt Vector Register
#define LAPIC_ISR_BASE          0x0100  // In-Service Register (8 32-bit registers)
#define LAPIC_TMR_BASE          0x0180  // Trigger Mode Register (8 32-bit registers)
#define LAPIC_IRR_BASE          0x0200  // Interrupt Request Register (8 32-bit registers)
#define LAPIC_ESR               0x0280  // Error Status Register
#define LAPIC_ICRLO             0x0300  // Interrupt Command Register (bits 0-31)
#define LAPIC_ICRHI             0x0310  // Interrupt Command Register (bits 32-63)
#define LAPIC_TIMER             0x0320  // LVT Timer Register
#define LAPIC_THERMAL           0x0330  // LVT Thermal Sensor Register
#define LAPIC_PERF              0x0340  // LVT Performance Monitor Register
#define LAPIC_LINT0             0x0350  // LVT LINT0 Register
#define LAPIC_LINT1             0x0360  // LVT LINT1 Register
#define LAPIC_ERROR             0x0370  // LVT Error Register
#define LAPIC_TIMER_ICR         0x0380  // Timer Initial Count Register
#define LAPIC_TIMER_CCR         0x0390  // Timer Current Count Register
#define LAPIC_TIMER_DCR         0x03E0  // Timer Divide Configuration Register

// Spurious Interrupt Vector Register bits
#define LAPIC_SVR_ENABLE        0x00000100  // APIC Software Enable/Disable
#define LAPIC_SVR_FOCUS         0x00000200  // Focus Processor Checking

// Timer Register bits
#define LAPIC_TIMER_PERIODIC    0x00020000  // Timer mode: periodic
#define LAPIC_TIMER_MASKED      0x00010000  // Interrupt masked

// Interrupt Command Register bits
#define LAPIC_ICR_INIT          0x00000500  // INIT IPI
#define LAPIC_ICR_STARTUP       0x00000600  // Startup IPI
#define LAPIC_ICR_DELIVS        0x00001000  // Delivery status
#define LAPIC_ICR_ASSERT        0x00004000  // Assert interrupt (vs deassert)
#define LAPIC_ICR_DEASSERT      0x00000000
#define LAPIC_ICR_LEVEL         0x00008000  // Level triggered
#define LAPIC_ICR_BCAST         0x00080000  // Send to all APICs, including self
#define LAPIC_ICR_BUSY          0x00001000
#define LAPIC_ICR_FIXED         0x00000000

// Timer divide values
#define LAPIC_TIMER_DIV_1       0x0B
#define LAPIC_TIMER_DIV_2       0x00
#define LAPIC_TIMER_DIV_4       0x01
#define LAPIC_TIMER_DIV_8       0x02
#define LAPIC_TIMER_DIV_16      0x03
#define LAPIC_TIMER_DIV_32      0x08
#define LAPIC_TIMER_DIV_64      0x09
#define LAPIC_TIMER_DIV_128     0x0A

// Interrupt vectors
#define LAPIC_SPURIOUS_VECTOR   0xFF
#define LAPIC_TIMER_VECTOR      32        // Timer interrupt vector
#define LAPIC_ERROR_VECTOR      51        // Error interrupt vector

/**
 * @brief Initialize the Local APIC for the current CPU
 * 
 * Sets up the Local APIC by:
 * - Mapping the APIC registers to memory
 * - Enabling the APIC via the Spurious Interrupt Vector Register
 * - Configuring the timer for periodic interrupts
 */
void lapic_init(void);

/**
 * @brief Send End-Of-Interrupt signal to Local APIC
 * 
 * Must be called at the end of every interrupt handler to signal
 * that interrupt processing is complete.
 */
void lapic_eoi(void);

/**
 * @brief Get the ID of the current Local APIC
 * 
 * @return uint8_t The APIC ID of the current processor
 */
uint8_t lapic_get_id(void);

/**
 * @brief Start the Local APIC timer in periodic mode
 * 
 * @param vector Interrupt vector to trigger
 * @param initial_count Initial count value for the timer
 */
void lapic_timer_start(uint8_t vector, uint32_t initial_count);

/**
 * @brief Stop the Local APIC timer
 */
void lapic_timer_stop(void);

/**
 * @brief Write to a Local APIC register
 * 
 * @param reg Register offset
 * @param value Value to write
 */
void lapic_write(uint32_t reg, uint32_t value);

/**
 * @brief Read from a Local APIC register
 * 
 * @param reg Register offset
 * @return uint32_t Value read from the register
 */
uint32_t lapic_read(uint32_t reg);

/**
 * @brief Check if Local APIC is available and initialized
 * 
 * @return bool True if APIC is available, false otherwise
 */
bool lapic_is_available(void);

#endif // SHIP_OS_LAPIC_H
