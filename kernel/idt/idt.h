//
// Created by oleg on 28.09.23.
// Copyright (c) 2023 untitled-os. All rights reserved.
//
// This header defines the structures and functions needed for setting up
// the Interrupt Descriptor Table (IDT) in x86_64 architecture.
//

#ifndef UNTITLED_OS_IDT_H
#define UNTITLED_OS_IDT_H

#include <inttypes.h>

#define NUM_IDT_ENTRIES 256  // Total number of entries in the IDT

/**
 * @brief IDTR structure used by lidt instruction
 * 
 * The IDTR points to the IDT and defines its size.
 */
struct IDTR {
    uint16_t limit;   // IDT size in bytes minus 1
    uint64_t base;    // Base address of the IDT
} __attribute__((packed));

/**
 * @brief 64-bit Interrupt Descriptor Table (IDT) entry
 * 
 * Represents a single gate descriptor in the IDT.
 * Each interrupt vector has one entry.
 */
struct InterruptDescriptor64 {
    uint16_t offset_1;       // Offset bits 0..15 of handler function
    uint16_t selector;       // Code segment selector in GDT or LDT
    uint8_t ist;             // Bits 0..2: Interrupt Stack Table index, rest must be zero
    uint8_t type_attributes; // Gate type, descriptor privilege level (DPL), present flag
    uint16_t offset_2;       // Offset bits 16..31 of handler function
    uint32_t offset_3;       // Offset bits 32..63 of handler function
    uint32_t zero;           // Reserved, must be zero
};

/**
 * @brief Set up the IDT, initialize entries, and load the IDTR
 * 
 * Initializes all 256 interrupt descriptors, assigns default and specific handlers
 * (timer, keyboard, CPU exceptions), and enables interrupts.
 */
void setup_idt();

#endif //UNTITLED_OS_IDT_H

