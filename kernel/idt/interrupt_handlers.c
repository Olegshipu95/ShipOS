//
// Created by ShipOS developers
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file contains interrupt handlers for the ShipOS kernel,
// including keyboard, timer, and default/unhandled interrupts.
// It also defines error messages for CPU exceptions.
//

#include "interrupt_handlers.h"

#include "../lib/include/x86_64.h"
#include "../lib/include/logging.h"
#include "../apic/lapic.h"
#include "../vga/vga.h"
#include "../tty/tty.h"
#include "../sched/scheduler.h"
#include "../sched/percpu.h"
#include "../sched/smp_sched.h"
#include "../pit/pit.h"

#define F1 0x3B

struct interrupt_frame;

/**
 * @brief Keyboard interrupt handler
 *
 * Handles key presses from the keyboard. Switches virtual terminals
 * when function keys F1-F7 are pressed. Other key codes are printed
 * to the current terminal.
 *
 * @param frame Pointer to interrupt frame (automatically passed by CPU)
 */
__attribute__((interrupt)) void keyboard_handler(struct interrupt_frame *frame)
{
    uint8_t status = inb(0x64);
    
    while (status & 1)
    {
        uint8_t res = inb(0x60);
        LOG_SERIAL("KEYBOARD", "Scancode: 0x%x on CPU %d", res, mycpu()->cpu_index);

        if (res >= F1 && res < F1 + TERMINALS_NUMBER)
        {
            set_tty(res - F1);
        }
        else
        {
            printf("%x ", res);
        }
        
        status = inb(0x64);
    }

    print("\n");

    lapic_eoi();
}

__attribute__((interrupt)) void default_handler(struct interrupt_frame *frame)
{
    print("unknown interrupt\n");
}

__attribute__((interrupt)) void timer_interrupt(struct interrupt_frame *frame)
{
    struct percpu *cpu = mycpu();

    // Increment per-CPU timer tick counter
    cpu->timer_ticks++;

    // Send EOI first to acknowledge the interrupt
    lapic_eoi();

    // Use SMP scheduler if ready, otherwise skip scheduling
    if (cpu->scheduler_ready)
    {
        sched_tick();
    }
}

/**
 * @brief CPU exception messages
 *
 * Provides human-readable descriptions of the first 32 CPU exceptions.
 */
char *error_messages[] = {
    "division_error",                 // 0
    "debug",                          // 1
    "non-maskable interrupt",         // 2
    "breakpoint",                     // 3
    "overflow",                       // 4
    "bound range exceeded",           // 5
    "invalid opcode",                 // 6
    "device not available",           // 7
    "double fault",                   // 8
    "pidor nahui blyat",              // 9
    "invalid tss",                    // 10
    "segment not present",            // 11
    "stack-segment fault",            // 12
    "general protection fault",       // 13
    "page fault",                     // 14
    "reserved",                       // 15
    "x87 floating-point exception",   // 16
    "alignment check",                // 17
    "machine check",                  // 18
    "simd floating-point exception",  // 19
    "virtualization exception",       // 20
    "control protection exception",   // 21
    "reserved",                       // 22
    "reserved",                       // 23
    "reserved",                       // 24
    "reserved",                       // 25
    "reserved",                       // 26
    "reserved",                       // 27
    "hypervisor injection exception", // 28
    "vmm communication exception",    // 29
    "security exception",             // 30
    "reserved"                        // 31
};

/**
 * @brief General interrupt handler for CPU exceptions
 *
 * Prints information about the interrupt number, human-readable
 * description, and error code. Also prints the CR2 register for
 * page fault exceptions. Halts the system after printing.
 *
 * @param error_code Error code provided by CPU (if any)
 * @param interrupt_number Number of the interrupt/exception
 */
void interrupt_handler(uint64_t error_code, uint64_t interrupt_number)
{
    LOG_SERIAL("EXCEPTION", "Interrupt %d (%s), error_code: 0x%lx, CR2: 0x%lx",
               interrupt_number, error_messages[interrupt_number], error_code, rcr2());
    printf("Interrupt number %d (%s), error_code: %b\n", interrupt_number, error_messages[interrupt_number], error_code);
    printf("CR2: %x\n", rcr2());
    while (1)
    {
    }
}
