//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Application Processor (AP) startup and management implementation
//

#include "ap_startup.h"
#include "lapic.h"
#include "../desc/madt.h"
#include "../lib/include/logging.h"
#include "../lib/include/memset.h"
#include "../lib/include/x86_64.h"
#include "../kalloc/kalloc.h"
#include "../paging/paging.h"
#include "../memlayout.h"

// External symbols from trampoline assembly
extern uint8_t ap_trampoline_start[];
extern uint8_t ap_trampoline_end[];

// Structure to hold trampoline data (matches assembly layout)
struct ap_trampoline_data {
    uint16_t gdt_limit;
    uint64_t gdt_base;
    uint64_t cr3;
    uint64_t stack;
    uint64_t entry;
} __attribute__((packed));

// Counter for number of APs that have started
static volatile uint32_t ap_started_count = 0;

/**
 * @brief Microsecond delay using LAPIC timer
 */
static void microdelay(uint32_t us)
{
    // Simple delay loop (very approximate)
    for (volatile uint32_t i = 0; i < us * 100; i++)
    {
        asm volatile("pause");
    }
}

/**
 * @brief Entry point for Application Processors
 */
void ap_entry(void)
{
    // Increment counter first
    __sync_fetch_and_add(&ap_started_count, 1);
    
    // Write a simple message to serial port directly (port 0x3F8)
    const char *msg = "[AP] Started\n";
    for (const char *p = msg; *p; p++)
    {
        // Wait for transmit buffer to be empty
        while (!(inb(0x3F8 + 5) & 0x20));
        outb(0x3F8, *p);
    }
    
    // Halt forever
    while (1)
    {
        asm volatile("cli; hlt");
    }
}

/**
 * @brief Copy trampoline code to low memory and set up data
 */
static void setup_trampoline(void *page_table)
{
    // Map low memory for trampoline (page at 0x8000)
    map_low_memory((pagetable_t)page_table, 0x0, PGSIZE * 16); // Map first 64KB
    LOG_SERIAL("AP", "Mapped low memory for trampoline");
    
    uint8_t *trampoline_dest = (uint8_t *)AP_TRAMPOLINE_ADDR;
    size_t trampoline_size = ap_trampoline_end - ap_trampoline_start;
    
    // Copy trampoline code to low memory
    for (size_t i = 0; i < trampoline_size; i++)
    {
        trampoline_dest[i] = ap_trampoline_start[i];
    }
    
    LOG_SERIAL("AP", "Copied trampoline code (%d bytes) to 0x%x", 
               trampoline_size, AP_TRAMPOLINE_ADDR);
    
    // Dump first few bytes for debugging
    LOG_SERIAL("AP", "Trampoline first bytes: 0x%02x 0x%02x 0x%02x 0x%02x",
               trampoline_dest[0], trampoline_dest[1], trampoline_dest[2], trampoline_dest[3]);
    
    // Use fixed offsets from the assembly file:
    // The trampoline has its own GDT, so we don't need to patch the GDT
    // 0xE0 = CR3 (8 bytes)
    // 0xE8 = Stack (8 bytes)
    // 0xF0 = Entry (8 bytes)
    
    uint64_t *cr3_ptr = (uint64_t *)(trampoline_dest + 0xE0);
    uint64_t *stack_ptr = (uint64_t *)(trampoline_dest + 0xE8);
    uint64_t *entry_ptr = (uint64_t *)(trampoline_dest + 0xF0);
    
    *cr3_ptr = (uint64_t)page_table;
    *stack_ptr = 0; // Will be set per-AP
    *entry_ptr = (uint64_t)ap_entry;
    
    LOG_SERIAL("AP", "CR3: 0x%llx, Entry: 0x%llx", (uint64_t)page_table, (uint64_t)ap_entry);
}

/**
 * @brief Start a single Application Processor
 */
static bool start_ap(uint8_t apic_id, void *stack)
{
    // Set stack pointer in trampoline at fixed offset 0xE8
    uint64_t *stack_ptr = (uint64_t *)(AP_TRAMPOLINE_ADDR + 0xE8);
    *stack_ptr = (uint64_t)stack + AP_STACK_SIZE;
    
    LOG_SERIAL("AP", "Starting AP with APIC ID %d, stack at 0x%llx", 
               apic_id, (uint64_t)stack);
    
    uint32_t old_count = ap_started_count;
    
    // Send INIT IPI
    lapic_send_init(apic_id);
    microdelay(10000); // 10ms delay
    
    // Send first SIPI (Startup IPI) with vector 0x08 (page 8 = address 0x8000)
    lapic_send_sipi(apic_id, 0x08);
    microdelay(200); // 200us delay
    
    // Send second SIPI
    lapic_send_sipi(apic_id, 0x08);
    microdelay(200); // 200us delay
    
    // Wait for AP to start (timeout after ~100ms)
    for (int i = 0; i < 1000; i++)
    {
        if (ap_started_count > old_count)
        {
            return true;
        }
        microdelay(100);
    }
    
    LOG_SERIAL("AP", "WARNING: AP %d did not start", apic_id);
    return false;
}

/**
 * @brief Initialize and start all Application Processors
 */
uint32_t start_all_aps(void *page_table)
{
    uint32_t cpu_count = get_cpu_count();
    uint32_t started = 0;
    
    LOG_SERIAL("AP", "Starting Application Processors (%d total CPUs)", cpu_count);
    
    // Set up trampoline code
    setup_trampoline(page_table);
    
    // Get our own APIC ID (BSP)
    uint8_t bsp_apic_id = lapic_get_id();
    LOG_SERIAL("AP", "BSP APIC ID: %d", bsp_apic_id);
    
    // Start each AP
    for (uint32_t i = 0; i < cpu_count; i++)
    {
        struct CPUInfo *cpu = get_cpu_info(i);
        if (cpu == NULL || !cpu->enabled)
        {
            continue;
        }
        
        // Skip BSP
        if (cpu->apic_id == bsp_apic_id)
        {
            LOG_SERIAL("AP", "Skipping BSP (APIC ID %d)", cpu->apic_id);
            continue;
        }
        
        // Allocate stack for this AP
        void *stack = kalloc();
        if (stack == NULL)
        {
            LOG_SERIAL("AP", "ERROR: Failed to allocate stack for AP %d", cpu->apic_id);
            continue;
        }
        
        // Clear the stack
        memset(stack, 0, AP_STACK_SIZE);
        
        // Start the AP
        if (start_ap(cpu->apic_id, stack))
        {
            started++;
        }
    }
    
    LOG_SERIAL("AP", "Started %d Application Processors", started);
    return started;
}
