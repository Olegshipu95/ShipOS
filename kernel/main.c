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
#include "lib/include/shutdown.h"
#include "paging/paging.h"
#include "sched/proc.h"
#include "sched/threads.h"
#include "sched/scheduler.h"
#include "sched/percpu.h"
#include "desc/rsdp.h"
#include "desc/rsdt.h"
#include "desc/madt.h"
#include "apic/ap_startup.h"

/**
 * @brief Initialize ACPI subsystem and map APIC memory regions
 * 
 * Initializes RSDP, RSDT, and MADT tables, then maps Local APIC
 * and all I/O APICs into the kernel page tables for MMIO access.
 * 
 * @param kernel_table Kernel page table to map APIC regions into
 */
static void init_acpi_and_map_apic(pagetable_t kernel_table)
{
    init_rsdp();
    if (get_rsdp() == NULL)
    {
        panic("Unable to initialize: ACPI unavailable");
    }
    
    init_rsdt(get_rsdp());
    init_madt();
    log_cpu_info();
    
    // Map Local APIC memory region
    uint32_t lapic_addr = get_lapic_address();
    if (lapic_addr != 0)
    {
        LOG_SERIAL("MEMORY", "Mapping Local APIC at 0x%x", lapic_addr);
        map_apic_region(kernel_table, lapic_addr, PGSIZE);
    }
    
    // Map all I/O APIC regions found in MADT
    struct MADT_t *madt = get_madt();
    if (madt != NULL)
    {
        uint8_t *entry_ptr = (uint8_t *)madt + sizeof(struct MADT_t);
        uint8_t *end_ptr = (uint8_t *)madt + madt->header.Length;
        
        while (entry_ptr < end_ptr)
        {
            struct MADTEntryHeader *header = (struct MADTEntryHeader *)entry_ptr;
            
            if (header->Type == MADT_ENTRY_IOAPIC)
            {
                struct MADTEntryIOAPIC *ioapic = (struct MADTEntryIOAPIC *)entry_ptr;
                LOG_SERIAL("MEMORY", "Mapping I/O APIC at 0x%x", ioapic->IOAPICAddr);
                map_apic_region(kernel_table, ioapic->IOAPICAddr, PGSIZE);
            }
            
            entry_ptr += header->Length;
        }
    }
}

// ============================================================================
// Test/Demo Thread Functions
// ============================================================================

/**
 * @brief Example function to repeatedly print a number from a thread.
 *
 * This is a simple demo of how threads can output information.
 * Currently, it loops infinitely printing "Hello from thread N".
 *
 * @param num Thread identifier number
 */
void print_num(uint32_t num)
{
    while (1)
    {
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
void thread_function(int argc, struct argument *args)
{
    uint32_t num = *((uint32_t *) args[0].value);
    print_num(num);
}

/**
 * @brief Kernel entry point
 *
 * Initialization sequence:
 * 1. Initialize CPU state
 * 2. Initialize serial ports for logging
 * 3. Initialize TTY terminals for console output
 * 4. Initialize physical memory allocator (kalloc)
 * 5. Set up kernel page tables with identity mapping
 * 6. Initialize ACPI subsystem and map APIC regions
 * 7. Complete physical memory initialization
 * 8. Initialize process and thread subsystems
 * 9. Set up Interrupt Descriptor Table with APIC
 * 10. Start the scheduler and enter idle loop
 *
 * @return int Never returns (enters infinite scheduler loop)
 */
int kernel_main()
{
    // Initialize serial ports first for early debugging
    int serial_ports_count = init_serial_ports();
    if (serial_ports_count == -1)
    {
        LOG("No serial ports detected");
    }
    else
    {
        LOG("Found %d serial port(s)", serial_ports_count);
        LOG("Using port %#x as default", get_default_serial_port());
        LOG_SERIAL("SERIAL", "Serial ports initialized successfully");
    }

    LOG("Kernel started");

    init_tty();
    for (uint8_t i = 0; i < TERMINALS_NUMBER; i++)
    {
        set_tty(i);
    }
    set_tty(0);
    LOG_SERIAL("BOOT", "TTY subsystem initialized");

    LOG(" CR3: %x", rcr3());
    LOG("Kernel end at address: %d", KEND);
    LOG("Kernel size: %d", KEND - KSTART);
    kinit(KEND, INIT_PHYSTOP);

    pagetable_t kernel_table = kvminit(INIT_PHYSTOP, PHYSTOP);
    LOG("kernel table: %p", kernel_table);

    // Initialize ACPI and map APIC regions
    init_acpi_and_map_apic(kernel_table);

    // Copy ACPI tables to safe memory before freeing upper memory region
    rsdt_copy_to_safe_memory();
    madt_copy_to_safe_memory();

    // Free upper memory region (INIT_PHYSTOP to PHYSTOP)
    kinit(INIT_PHYSTOP, PHYSTOP);
    LOG("Successfully allocated physical memory up to %p", PHYSTOP);
    LOG_SERIAL("MEMORY", "Physical memory initialized");

    // Initialize per-CPU data structures for BSP
    uint32_t cpu_count = get_cpu_count();
    percpu_init_bsp(cpu_count);
    
    // Allocate per-CPU stacks (requires kalloc to be initialized)
    percpu_alloc_stacks();
    LOG_SERIAL("PERCPU", "Per-CPU data structures initialized for %d CPUs", cpu_count);

    int pages = count_pages();

    struct proc_node *init_proc_node = procinit();
    struct thread *init_thread = peek_thread_list(init_proc_node->data->threads);

    setup_idt();
    LOG_SERIAL("KERNEL", "Boot sequence completed successfully");

    // Start Application Processors
    uint32_t ap_count = start_all_aps(kernel_table);
    LOG_SERIAL("KERNEL", "Started %d Application Processors", ap_count);

    // Log per-CPU data after all CPUs are initialized
    percpu_log_cpu_info();

#ifdef TEST
    run_tests();
    shutdown();
#endif

    LOG("Entering idle loop...");
    
    for (volatile int i = 0; i < 50000000; i++);  // Simple delay
    percpu_log_timer_ticks();

    for (volatile int i = 0; i < 50000000; i++);  // Simple delay
    percpu_log_timer_ticks();

    // scheduler();

    while (1)
    {
    };
    return 0;
}
