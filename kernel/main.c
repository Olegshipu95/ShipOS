//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Main kernel entry point and thread utilities
//

#include "vga/vga.h"
#include "idt/idt.h"
#include "tty/tty.h"
#include "serial/serial.h"
#include "kalloc/kalloc.h"
#include "lib/include/panic.h"
#include "memlayout.h"
#include "lib/include/x86_64.h"
#include "paging/paging.h"
#include "sched/proc.h"
#include "sched/threads.h"
#include "sched/scheduler.h"


/**
 * @brief Example function to repeatedly print a number from a thread.
 * 
 * This is a simple demo of how threads can output information. 
 * Currently, it loops infinitely printing "Hello from thread N".
 * 
 * @param num Thread identifier number
 */
void print_num(uint32_t num) {
    while (1) {
        printf("Hello from thread %d\r\n", num);
        // yield();
    }
}

/**
 * @brief Entry point for a created thread.
 * 
 * Extracts the integer argument from the provided arguments array 
 * and calls print_num with that value.
 * Made for testing functionality
 * 
 * @param argc Number of arguments
 * @param args Array of thread arguments
 */
void thread_function(int argc, struct argument *args) {
    uint32_t num = *((uint32_t*) args[0].value);
    print_num(num);
}


/**
 * @brief Kernel entry point.
 * 
 * Performs basic initialization:
 * 1. Initializes and sets up serial port
 * 2. Sets up TTY terminals.
 * 3. Prints debug information about CR3 register and kernel memory layout.
 * 4. Initializes the physical memory allocator and page tables.
 * 5. Initializes the first process and its main thread.
 * 6. Sets up the Interrupt Descriptor Table (IDT).
 * 7. Starts the scheduler (currently commented out for testing).
 * 
 * @return int Always returns 0 (never reached).
 */
int kernel_main(){
    // Initialize serial port FIRST for early boot logging
    if (init_serial() == -1) {
        printf("Warning! Faulty serial port, output to it won't work!\n");
    }
    LOG_BOOT_SERIAL("Kernel started");

    init_tty();
    LOG_BOOT_SERIAL("TTY subsystem initialized");

    for (uint8_t i=0; i < TERMINALS_NUMBER; i++) {
        set_tty(i);
        printf("TTY %d\n", i);
        LOG_BOOT_SERIAL("TTY %d initialized", i);
    }
    set_tty(0);

    LOG_BOOT_SERIAL("CR3 register: %x", rcr3());
    printf(" CR3: %x\n", rcr3());

    print("$ \r\n");

    LOG_BOOT_SERIAL("Kernel start: %p, end: %p", KSTART, KEND);
    LOG_BOOT_SERIAL("Kernel size: %d bytes", KEND - KSTART);

    printf("Kernel end at address: %d\n", KEND);
    printf("Kernel size: %d\n", KEND - KSTART);

    LOG_MEM_SERIAL("Initializing physical memory allocator (phase 1)...");
    kinit(KEND, INIT_PHYSTOP);

    LOG_MEM_SERIAL("Setting up kernel page table...");
    pagetable_t kernel_table = kvminit(INIT_PHYSTOP, PHYSTOP);
    printf("kernel table: %p\n", kernel_table);
    LOG_MEM_SERIAL("Kernel page table at: %p", kernel_table);

    LOG_MEM_SERIAL("nitializing physical memory allocator (phase 2)...");
    kinit(INIT_PHYSTOP, PHYSTOP);
    printf("Successfully allocated physical memory up to %p\n", PHYSTOP);
    LOG_MEM_SERIAL("Physical memory initialized up to: %p", PHYSTOP);

    int pages = count_pages();
    printf("%d pages available in allocator\n", pages);
    LOG_MEM_SERIAL("Available pages: %d", pages);

    LOG_PROC_SERIAL("Initializing first process...");
    struct proc_node *init_proc_node = procinit();
    printf("Init proc node %p\n", init_proc_node);
    LOG_PROC_SERIAL("Init process node created at: %p", init_proc_node);

    struct thread *init_thread = peek_thread_list(init_proc_node->data->threads);
    printf("Got init thread\n");
    LOG_PROC_SERIAL("Init thread retrieved successfully");

    LOG_INT_SERIAL("Setting up IDT...");
    setup_idt();
    LOG_INT_SERIAL("IDT initialized");

    LOG_KER_SERIAL("Boot sequence completed successfully");
    LOG_KER_SERIAL("Entering idle loop...");

    // scheduler();

    while(1) {};
    return 0;
}
