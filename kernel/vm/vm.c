//
// Created by ShipOS developers
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Virtual memory definitions and utility macros for ShipOS kernel.
//

/**
 * @brief EFLAGS register bit for enabling interrupts.
 *
 * The FL_INT bit in the EFLAGS register controls the CPU's interrupt flag.
 * When set, the processor responds to maskable hardware interrupts.
 *
 * Usage:
 *   pushf
 *   or word [flags], FL_INT
 *   popf
 */
#define FL_INT 0x00000200 // Interrupt Enable
