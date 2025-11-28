//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Main kernel entry point and thread utilities
//

#include "vga/vga.h"
#include "idt/idt.h"
#include "kalloc/kalloc.h"
#include "lib/include/panic.h"
#include "memlayout.h"
#include "lib/include/x86_64.h"
#include "lib/include/logging.h"
#include "lib/include/test.h"
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
    // Initialize serial ports
    uint16_t serial_ports[MAX_SERIAL_PORTS];
    int serial_ports_count = detect_serial_ports(serial_ports);
    if (serial_ports_count == 0) {
        LOG("No serial ports detected");
    } else {
        set_default_serial_port(serial_ports[0]);
        LOG("Found %d serial port(s)", serial_ports_count);
        LOG("Using port 0x%p as default", serial_ports[0]);
    }

    LOG("Kernel started");

    init_tty();
    for (uint8_t i=0; i < TERMINALS_NUMBER; i++) {
        set_tty(i);
    }
    set_tty(0);

    LOG(" CR3: %x", rcr3());
    LOG("Kernel end at address: %d", KEND);
    LOG("Kernel size: %d", KEND - KSTART);
    kinit(KEND, INIT_PHYSTOP);

    pagetable_t kernel_table = kvminit(INIT_PHYSTOP, PHYSTOP);
    LOG("kernel table: %p", kernel_table);
    kinit(INIT_PHYSTOP, PHYSTOP);
    LOG("Successfully allocated physical memory up to %p", PHYSTOP);

    int pages = count_pages();
    struct proc_node *init_proc_node = procinit();
    struct thread *init_thread = peek_thread_list(init_proc_node->data->threads);
    setup_idt();

    LOG("Boot sequence completed successfully");

#ifdef TEST
    run_tests();
#endif

    LOG("Entering idle loop...");

    // scheduler();

    while(1) {};
    return 0;
}
