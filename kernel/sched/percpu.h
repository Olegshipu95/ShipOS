//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Per-CPU data structures for SMP support.
// Each CPU has its own private data area accessed via GS segment or APIC ID indexing.
//

#ifndef SHIP_OS_PERCPU_H
#define SHIP_OS_PERCPU_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include "threads.h"

// Maximum number of CPUs supported
#define MAX_CPUS 64

// ============================================================================
// Task State Segment (TSS) for x86_64
// ============================================================================

/**
 * @brief 64-bit Task State Segment
 * 
 * In long mode, TSS is primarily used for:
 * - Storing IST (Interrupt Stack Table) pointers for stack switching on interrupts
 * - RSP0 for privilege level changes (ring 3 -> ring 0)
 */
struct tss64 {
    uint32_t reserved0;
    uint64_t rsp0;           // Stack pointer for privilege level 0 (kernel)
    uint64_t rsp1;           // Stack pointer for privilege level 1 (unused)
    uint64_t rsp2;           // Stack pointer for privilege level 2 (unused)
    uint64_t reserved1;
    uint64_t ist1;           // Interrupt Stack Table entry 1
    uint64_t ist2;           // Interrupt Stack Table entry 2
    uint64_t ist3;           // Interrupt Stack Table entry 3
    uint64_t ist4;           // Interrupt Stack Table entry 4
    uint64_t ist5;           // Interrupt Stack Table entry 5
    uint64_t ist6;           // Interrupt Stack Table entry 6
    uint64_t ist7;           // Interrupt Stack Table entry 7
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;    // I/O Map Base Address (offset from TSS base)
} __attribute__((packed));

// ============================================================================
// GDT Entry structures
// ============================================================================

/**
 * @brief Standard 64-bit GDT entry (code/data segments)
 */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

/**
 * @brief System segment descriptor for TSS (16 bytes in 64-bit mode)
 */
struct gdt_tss_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle1;
    uint8_t  access;
    uint8_t  limit_high_flags;
    uint8_t  base_middle2;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));

/**
 * @brief GDT Pointer structure for lgdt instruction
 */
struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// ============================================================================
// Per-CPU Data Structure
// ============================================================================

/**
 * @brief Per-CPU data structure
 * 
 * Contains all CPU-local state that should not be shared between processors.
 */
struct percpu {
    // Self-pointer for fast access via GS segment
    struct percpu *self;
    
    // CPU identification
    uint32_t apic_id;           // Local APIC ID
    uint32_t cpu_index;         // Index in cpus[] array (0 = BSP)
    bool     is_bsp;            // Is this the Bootstrap Processor?
    bool     started;           // Has this CPU finished initialization?
    
    // Interrupt state
    int      ncli;              // Depth of pushcli nesting
    int      intena;            // Were interrupts enabled before pushcli?
    
    // Scheduler state
    struct thread *current_thread;  // Currently running thread on this CPU
    struct thread *idle_thread;     // Idle thread for this CPU
    
    // Timer interrupt counter for debugging
    volatile uint64_t timer_ticks;  // Number of timer interrupts received
    
    // TSS for this CPU
    struct tss64 tss;
    
    // Interrupt stack (used by IST)
    uint8_t *int_stack;
    
    // Kernel stack for this CPU's scheduler context
    uint8_t *kstack;
    
    // GDT for this CPU (includes TSS entry)
    // Layout: null, kernel code, kernel data, user code, user data, TSS (16 bytes)
    uint8_t gdt[8 * 8 + 16] __attribute__((aligned(16)));
    struct gdt_ptr gdt_ptr;
    
    // Padding to cache line boundary (avoid false sharing)
    uint8_t padding[64];
} __attribute__((aligned(64)));

// ============================================================================
// Global Variables
// ============================================================================

// Array of all per-CPU data structures (named 'percpus' to avoid conflict with madt.c)
extern struct percpu percpus[MAX_CPUS];

// Number of CPUs detected and initialized
extern uint32_t ncpu;

// Flag indicating if SMP is initialized
extern bool smp_initialized;

// ============================================================================
// Per-CPU Access Functions
// ============================================================================

/**
 * @brief Get the current CPU's APIC ID using CPUID
 * @return APIC ID of the executing CPU
 */
static inline uint32_t get_apic_id(void) {
    uint32_t eax, ebx, ecx, edx;
    
    // CPUID leaf 1: EBX[31:24] contains initial APIC ID
    asm volatile("cpuid" 
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
                 : "a"(1) 
                 : "memory");
    
    return ebx >> 24;
}

/**
 * @brief Get the current CPU's APIC ID from the Local APIC register
 * @param lapic_base Virtual address of Local APIC registers
 * @return APIC ID of the executing CPU
 * 
 * This is more reliable than CPUID on some systems.
 */
static inline uint32_t get_apic_id_from_lapic(volatile uint32_t *lapic_base) {
    // LAPIC ID register is at offset 0x20
    return lapic_base[0x20 / 4] >> 24;
}

/**
 * @brief Get pointer to current CPU's percpu structure
 * 
 * Uses the APIC ID to index into the cpus array.
 * Must be called with interrupts disabled or from a context where
 * preemption cannot occur.
 * 
 * @return Pointer to current CPU's percpu structure
 */
struct percpu *mycpu(void);

/**
 * @brief Get pointer to percpu structure by CPU index
 * @param index CPU index (0 = BSP, 1+ = APs)
 * @return Pointer to the CPU's percpu structure, or NULL if invalid
 */
struct percpu *cpu_by_index(uint32_t index);

/**
 * @brief Get pointer to percpu structure by APIC ID
 * @param apic_id Local APIC ID
 * @return Pointer to the CPU's percpu structure, or NULL if not found
 */
struct percpu *cpu_by_apic_id(uint32_t apic_id);

// ============================================================================
// Per-CPU Initialization
// ============================================================================

/**
 * @brief Initialize per-CPU data for the Bootstrap Processor
 * 
 * Must be called early in boot, before APs are started.
 * Sets up BSP's percpu structure, GDT with TSS, and loads it.
 * 
 * @param total_cpus Total number of CPUs detected from MADT
 */
void percpu_init_bsp(uint32_t total_cpus);

/**
 * @brief Initialize per-CPU data for an Application Processor
 * 
 * Called by each AP during its initialization.
 * Sets up the AP's percpu structure, GDT with TSS, and loads it.
 * 
 * @param cpu_index Index assigned to this CPU
 */
void percpu_init_ap(uint32_t cpu_index);

/**
 * @brief Allocate and set up per-CPU stacks
 * 
 * Must be called after memory allocator is initialized.
 * Allocates interrupt stack and kernel stack for each CPU.
 */
void percpu_alloc_stacks(void);

/**
 * @brief Log detailed information about all CPU data structures
 * 
 * Outputs per-CPU info including APIC IDs, stack addresses,
 * TSS configuration, and GDT locations.
 */
void percpu_log_cpu_info(void);

/**
 * @brief Set up GDT with TSS for a specific CPU
 * @param cpu Pointer to the CPU's percpu structure
 */
void percpu_setup_gdt(struct percpu *cpu);

/**
 * @brief Load the CPU's GDT and TSS
 * @param cpu Pointer to the CPU's percpu structure
 */
void percpu_load_gdt(struct percpu *cpu);

// ============================================================================
// Interrupt State Management
// ============================================================================

/**
 * @brief Push CLI - disable interrupts and track nesting depth
 * 
 * Safe to call multiple times; will only re-enable interrupts
 * when all pushcli calls have been matched with popcli.
 */
void pushcli(void);

/**
 * @brief Pop CLI - potentially re-enable interrupts
 * 
 * Only re-enables interrupts if this is the last popcli and
 * interrupts were enabled before the first pushcli.
 */
void popcli(void);

/**
 * @brief Log timer tick counts for all CPUs
 * 
 * Prints a summary of timer interrupts received by each CPU.
 * Useful for verifying SMP timer interrupt distribution.
 */
void percpu_log_timer_ticks(void);

// ============================================================================
// Convenience Macros
// ============================================================================

/**
 * @brief Get the current thread running on this CPU
 * @return Pointer to current thread, or NULL if no thread scheduled
 */
#define curthread() (mycpu()->current_thread)

/**
 * @brief Check if we're on the Bootstrap Processor
 */
#define is_bsp() (mycpu()->is_bsp)

/**
 * @brief Get current CPU's index
 */
#define cpunum() (mycpu()->cpu_index)

#endif // SHIP_OS_PERCPU_H
