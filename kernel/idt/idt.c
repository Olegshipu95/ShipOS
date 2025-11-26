//
// Created by oleg on 28.09.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// This file sets up the Interrupt Descriptor Table (IDT) for x86_64 architecture.
// It initializes all interrupt descriptors and points the CPU to the IDT.
// Specific handlers for timer, keyboard, and CPU exceptions are registered here.
//

#include "idt.h"
#include "interrupt_handlers.h"
#include "../lib/include/memset.h"
#include "../pic/pic.h"
#include "../pit/pit.h"

#define MAX_INTERRUPTS 256 // Total number of interrupt vectors

/**
 * @brief Fill a single IDT entry with the handler information
 *
 * @param idt Pointer to the IDT array
 * @param array_index Index of the interrupt vector to fill
 * @param handler Address of the interrupt handler
 */
void make_interrupt(struct InterruptDescriptor64* idt, int array_index, uintptr_t handler){
    if(array_index>=MAX_INTERRUPTS)return;
    idt[array_index].offset_1 = (uint16_t)handler;
    idt[array_index].selector = 0x08;               // Kernel code segment selector
    idt[array_index].type_attributes = 0x8E;        // Interrupt gate, present, DPL=0
    idt[array_index].offset_2 = (uint16_t)((handler) >> 16);
    idt[array_index].offset_3 = (uint32_t)((handler) >> 32);
}

/**
 * @brief Set up the IDT, initialize entries, and load the IDTR
 *
 * Initializes all 256 interrupt descriptors, assigns default and specific handlers
 * (timer, keyboard, CPU exceptions), and enables interrupts.
 */
void setup_idt(){
    struct InterruptDescriptor64 idt[MAX_INTERRUPTS]; // Array for 256 IDT entries

    // Configure IDTR (IDT register)
    struct IDTR idtr;
    idtr.limit = sizeof(struct InterruptDescriptor64) * MAX_INTERRUPTS - 1;
    idtr.base = (uint64_t)&idt;

    // Clear the IDT
    memset(&idt, 0, sizeof(struct InterruptDescriptor64) * MAX_INTERRUPTS);

    // Set all entries to default_handler initially
    for (int i = 0; i < MAX_INTERRUPTS; ++i) {
        make_interrupt(idt, i, (uintptr_t)default_handler);
    }

    // Setup specific hardware interrupt handlers
    make_interrupt(idt, PIC_MASTER_OFFSET, (uintptr_t)timer_interrupt);
    make_interrupt(idt, PIC_MASTER_OFFSET+1, (uintptr_t)keyboard_handler);

    // Setup CPU exception handlers (vectors 0-31)
    make_interrupt(idt, 0, (uintptr_t)interrupt_handler_0);
    make_interrupt(idt, 1, (uintptr_t)interrupt_handler_1);
    make_interrupt(idt, 2, (uintptr_t)interrupt_handler_2);
    make_interrupt(idt, 3, (uintptr_t)interrupt_handler_3);
    make_interrupt(idt, 4, (uintptr_t)interrupt_handler_4);
    make_interrupt(idt, 5, (uintptr_t)interrupt_handler_5);
    make_interrupt(idt, 6, (uintptr_t)interrupt_handler_6);
    make_interrupt(idt, 7, (uintptr_t)interrupt_handler_7);
    make_interrupt(idt, 8, (uintptr_t)interrupt_handler_8);
    make_interrupt(idt, 9, (uintptr_t)interrupt_handler_9);
    make_interrupt(idt, 10, (uintptr_t)interrupt_handler_10);
    make_interrupt(idt, 11, (uintptr_t)interrupt_handler_11);
    make_interrupt(idt, 12, (uintptr_t)interrupt_handler_12);
    make_interrupt(idt, 13, (uintptr_t)interrupt_handler_13);
    make_interrupt(idt, 14, (uintptr_t)interrupt_handler_14);
    make_interrupt(idt, 15, (uintptr_t)interrupt_handler_15);
    make_interrupt(idt, 16, (uintptr_t)interrupt_handler_16);
    make_interrupt(idt, 17, (uintptr_t)interrupt_handler_17);
    make_interrupt(idt, 18, (uintptr_t)interrupt_handler_18);
    make_interrupt(idt, 19, (uintptr_t)interrupt_handler_19);
    make_interrupt(idt, 20, (uintptr_t)interrupt_handler_20);
    make_interrupt(idt, 21, (uintptr_t)interrupt_handler_21);
    make_interrupt(idt, 22, (uintptr_t)interrupt_handler_22);
    make_interrupt(idt, 23, (uintptr_t)interrupt_handler_23);
    make_interrupt(idt, 24, (uintptr_t)interrupt_handler_24);
    make_interrupt(idt, 25, (uintptr_t)interrupt_handler_25);
    make_interrupt(idt, 26, (uintptr_t)interrupt_handler_26);
    make_interrupt(idt, 27, (uintptr_t)interrupt_handler_27);
    make_interrupt(idt, 28, (uintptr_t)interrupt_handler_28);
    make_interrupt(idt, 29, (uintptr_t)interrupt_handler_29);
    make_interrupt(idt, 30, (uintptr_t)interrupt_handler_30);
    make_interrupt(idt, 31, (uintptr_t)interrupt_handler_31);

    // Load IDTR register
    asm volatile ("lidt %0" : : "m"(idtr));

    pic_init();
    init_pit();

    // Mask all IRQs initially
    outb(PIC1_DATA,0xff);
    outb(PIC2_DATA,0xff);

    // Unmask timer and keyboard IRQs
    outb(PIC1_DATA, ~(1<<0 | 1<<1));

    // Enable interrupts
    asm("sti");
}
