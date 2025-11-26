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
 * 1. Sets up TTY terminals.
 * 2. Prints debug information about CR3 register and kernel memory layout.
 * 3. Initializes the physical memory allocator and page tables.
 * 4. Initializes the first process and its main thread.
 * 5. Sets up the Interrupt Descriptor Table (IDT).
 * 6. Starts the scheduler (currently commented out for testing).
 * 
 * @return int Always returns 0 (never reached).
 */
int kernel_main(){
    // Initialize serial port FIRST for early boot logging
    init_serial();
    serial_write("[BOOT] Kernel started\r\n");

    init_tty();
    serial_printf("[BOOT] TTY subsystem initialized\r\n");

    for (uint8_t i=0; i < TERMINALS_NUMBER; i++) {
        set_tty(i);
        printf("TTY %d\n", i);
        serial_printf("[BOOT] TTY %d initialized\r\n", i);
    }
    set_tty(0);

    serial_printf("[BOOT] CR3 register: %x\r\n", rcr3());
    printf(" CR3: %x\n", rcr3());

    print("$ \r\n");

    serial_printf("[BOOT] Kernel start: %p, end: %p\r\n", KSTART, KEND);
    serial_printf("[BOOT] Kernel size: %d bytes\r\n", KEND - KSTART);
    printf("Kernel end at address: %d\n", KEND);
    printf("Kernel size: %d\n", KEND - KSTART);

    serial_printf("[MEMORY] Initializing physical memory allocator (phase 1)...\r\n");
    kinit(KEND, INIT_PHYSTOP);

    serial_printf("[MEMORY] Setting up kernel page table...\r\n");
    pagetable_t kernel_table = kvminit(INIT_PHYSTOP, PHYSTOP);
    printf("kernel table: %p\n", kernel_table);
    serial_printf("[MEMORY] Kernel page table at: %p\r\n", kernel_table);

    serial_printf("[MEMORY] Initializing physical memory allocator (phase 2)...\r\n");
    kinit(INIT_PHYSTOP, PHYSTOP);
    printf("Successfully allocated physical memory up to %p\n", PHYSTOP);
    serial_printf("[MEMORY] Physical memory initialized up to: %p\r\n", PHYSTOP);

    int pages = count_pages();
    printf("%d pages available in allocator\n", pages);
    serial_printf("[MEMORY] Available pages: %d\r\n", pages);

    serial_printf("[PROCESS] Initializing first process...\r\n");
    struct proc_node *init_proc_node = procinit();
    printf("Init proc node %p\n", init_proc_node);
    serial_printf("[PROCESS] Init process node created at: %p\r\n", init_proc_node);

    struct thread *init_thread = peek_thread_list(init_proc_node->data->threads);
    printf("Got init thread\n");
    serial_printf("[PROCESS] Init thread retrieved successfully\r\n");

    serial_printf("[INTERRUPT] Setting up IDT...\r\n");
    setup_idt();
    serial_printf("[INTERRUPT] IDT initialized\r\n");

    serial_printf("[KERNEL] Boot sequence completed successfully\r\n");
    serial_printf("[KERNEL] Entering idle loop...\r\n");

    // scheduler();

    while(1) {};
    return 0;
}
