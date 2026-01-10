//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// Per-CPU data structure implementation for SMP support.
//

#include "percpu.h"
#include "../lib/include/memset.h"
#include "../lib/include/logging.h"
#include "../kalloc/kalloc.h"
#include "../lib/include/x86_64.h"

// ============================================================================
// Global Variables
// ============================================================================

// Array of all per-CPU data structures
struct percpu percpus[MAX_CPUS];

// Number of CPUs detected
uint32_t ncpu = 0;

// Mapping from APIC ID to CPU index
static int apic_to_cpu[256];  // APIC IDs are 8-bit

// Flag indicating if SMP is initialized
bool smp_initialized = false;

// ============================================================================
// GDT Segment Selectors
// ============================================================================

#define GDT_NULL        0x00
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18
#define GDT_USER_DATA   0x20
#define GDT_TSS         0x28  // TSS takes 2 entries (16 bytes)

// GDT access byte flags
#define GDT_PRESENT     (1 << 7)
#define GDT_DPL0        (0 << 5)
#define GDT_DPL3        (3 << 5)
#define GDT_CODE        (1 << 4) | (1 << 3)  // Executable, code segment
#define GDT_DATA        (1 << 4)              // Data segment
#define GDT_TSS_TYPE    0x89                  // 64-bit TSS (available)

// GDT granularity flags
#define GDT_LONG_MODE   (1 << 5)  // 64-bit code segment
#define GDT_DB          (1 << 6)  // Default operation size
#define GDT_GRANULARITY (1 << 7)  // 4KB granularity

// ============================================================================
// Per-CPU Access Functions
// ============================================================================

// Early boot per-CPU structure for BSP before full initialization
static struct percpu early_bsp_percpu = {0};
static bool percpu_fully_initialized = false;

struct percpu *mycpu(void) {
    // Before percpu system is fully initialized, return early boot structure
    if (!percpu_fully_initialized) {
        return &early_bsp_percpu;
    }
    
    // Get APIC ID and look up CPU
    uint32_t apic_id = get_apic_id();
    int idx = apic_to_cpu[apic_id];
    
    if (idx < 0 || idx >= (int)ncpu) {
        // Fallback to linear search if mapping not set up yet
        for (uint32_t i = 0; i < ncpu; i++) {
            if (percpus[i].apic_id == apic_id) {
                return &percpus[i];
            }
        }
        // Return BSP as fallback during early boot
        return &percpus[0];
    }
    
    return &percpus[idx];
}

struct percpu *cpu_by_index(uint32_t index) {
    if (index >= ncpu) {
        return (void *)0;
    }
    return &percpus[index];
}

struct percpu *cpu_by_apic_id(uint32_t apic_id) {
    if (apic_id > 255) {
        return (void *)0;
    }
    int idx = apic_to_cpu[apic_id];
    if (idx < 0 || idx >= (int)ncpu) {
        return (void *)0;
    }
    return &percpus[idx];
}

// ============================================================================
// GDT Setup
// ============================================================================

/**
 * @brief Create a standard GDT entry
 */
static void set_gdt_entry(struct gdt_entry *entry, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t flags) {
    entry->limit_low = limit & 0xFFFF;
    entry->base_low = base & 0xFFFF;
    entry->base_middle = (base >> 16) & 0xFF;
    entry->access = access;
    entry->granularity = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    entry->base_high = (base >> 24) & 0xFF;
}

/**
 * @brief Create a TSS entry (16 bytes in 64-bit mode)
 */
static void set_tss_entry(struct gdt_tss_entry *entry, uint64_t base, uint32_t limit) {
    entry->limit_low = limit & 0xFFFF;
    entry->base_low = base & 0xFFFF;
    entry->base_middle1 = (base >> 16) & 0xFF;
    entry->access = GDT_TSS_TYPE;
    entry->limit_high_flags = ((limit >> 16) & 0x0F);
    entry->base_middle2 = (base >> 24) & 0xFF;
    entry->base_high = (base >> 32) & 0xFFFFFFFF;
    entry->reserved = 0;
}

void percpu_setup_gdt(struct percpu *cpu) {
    struct gdt_entry *gdt = (struct gdt_entry *)cpu->gdt;
    
    // Entry 0: NULL descriptor
    memset(&gdt[0], 0, sizeof(struct gdt_entry));
    
    // Entry 1: Kernel Code Segment (0x08)
    // 64-bit code segment: present, DPL 0, executable, readable
    set_gdt_entry(&gdt[1], 0, 0xFFFFF,
                  GDT_PRESENT | GDT_DPL0 | GDT_CODE | 0x02,  // +readable
                  GDT_LONG_MODE | GDT_GRANULARITY);
    
    // Entry 2: Kernel Data Segment (0x10)
    // Data segment: present, DPL 0, writable
    set_gdt_entry(&gdt[2], 0, 0xFFFFF,
                  GDT_PRESENT | GDT_DPL0 | GDT_DATA | 0x02,  // +writable
                  GDT_GRANULARITY);
    
    // Entry 3: User Code Segment (0x18) - for future use
    set_gdt_entry(&gdt[3], 0, 0xFFFFF,
                  GDT_PRESENT | GDT_DPL3 | GDT_CODE | 0x02,
                  GDT_LONG_MODE | GDT_GRANULARITY);
    
    // Entry 4: User Data Segment (0x20) - for future use
    set_gdt_entry(&gdt[4], 0, 0xFFFFF,
                  GDT_PRESENT | GDT_DPL3 | GDT_DATA | 0x02,
                  GDT_GRANULARITY);
    
    // Entry 5-6: TSS (0x28) - takes 16 bytes (2 entries)
    struct gdt_tss_entry *tss_entry = (struct gdt_tss_entry *)&gdt[5];
    set_tss_entry(tss_entry, (uint64_t)&cpu->tss, sizeof(struct tss64) - 1);
    
    // Set up GDT pointer
    cpu->gdt_ptr.limit = sizeof(cpu->gdt) - 1;
    cpu->gdt_ptr.base = (uint64_t)cpu->gdt;
}

void percpu_load_gdt(struct percpu *cpu) {
    // Load GDT
    asm volatile("lgdt %0" : : "m"(cpu->gdt_ptr));
    
    // Reload segment registers
    asm volatile(
        "mov %0, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%ss\n\t"
        : : "i"(GDT_KERNEL_DATA) : "ax"
    );
    
    // Reload CS with a far jump (using retfq trick)
    asm volatile(
        "pushq %0\n\t"          // Push new CS
        "leaq 1f(%%rip), %%rax\n\t"  // Load address of label 1
        "pushq %%rax\n\t"       // Push new RIP
        "lretq\n\t"             // Far return to reload CS
        "1:\n\t"
        : : "i"((uint64_t)GDT_KERNEL_CODE) : "rax", "memory"
    );
    
    // Load TSS
    uint16_t tss_sel = GDT_TSS;
    asm volatile("ltr %0" : : "r"(tss_sel));
}

// ============================================================================
// TSS Setup
// ============================================================================

static void setup_tss(struct percpu *cpu) {
    memset(&cpu->tss, 0, sizeof(struct tss64));
    
    // RSP0: Stack pointer for privilege level 0 transitions
    // This is used when switching from user mode to kernel mode
    if (cpu->kstack) {
        cpu->tss.rsp0 = (uint64_t)(cpu->kstack + 4096);  // Stack grows down
    }
    
    // IST1: Interrupt Stack for critical handlers (double fault, NMI, etc.)
    if (cpu->int_stack) {
        cpu->tss.ist1 = (uint64_t)(cpu->int_stack + 4096);
    }
    
    // I/O Permission Bitmap offset (disable I/O ports from user mode)
    cpu->tss.iopb_offset = sizeof(struct tss64);
}

// ============================================================================
// Per-CPU Initialization
// ============================================================================

void percpu_init_bsp(uint32_t total_cpus) {
    // Initialize APIC ID to CPU mapping
    for (int i = 0; i < 256; i++) {
        apic_to_cpu[i] = -1;
    }
    
    ncpu = total_cpus;
    if (ncpu > MAX_CPUS) {
        ncpu = MAX_CPUS;
    }
    
    // Initialize BSP (CPU 0)
    struct percpu *bsp = &percpus[0];
    memset(bsp, 0, sizeof(struct percpu));
    
    bsp->self = bsp;
    bsp->apic_id = get_apic_id();
    bsp->cpu_index = 0;
    bsp->is_bsp = true;
    bsp->started = true;
    bsp->ncli = 0;
    bsp->intena = 0;
    bsp->current_thread = (void *)0;
    bsp->idle_thread = (void *)0;
    
    // Set up APIC ID mapping
    apic_to_cpu[bsp->apic_id] = 0;
    
    // Mark per-CPU system as fully initialized
    percpu_fully_initialized = true;
    
    LOG_SERIAL("PERCPU", "BSP initialized: APIC ID %d, CPU index 0", bsp->apic_id);
}

void percpu_alloc_stacks(void) {
    // Allocate stacks for all CPUs
    for (uint32_t i = 0; i < ncpu; i++) {
        struct percpu *cpu = &percpus[i];
        
        // Allocate interrupt stack (for IST)
        cpu->int_stack = (uint8_t *)kalloc();
        if (cpu->int_stack) {
            memset(cpu->int_stack, 0, 4096);
        }
        
        // Allocate kernel scheduler stack
        cpu->kstack = (uint8_t *)kalloc();
        if (cpu->kstack) {
            memset(cpu->kstack, 0, 4096);
        }
        
        // Set up TSS with the new stacks
        setup_tss(cpu);
        
        // Set up and load GDT for BSP immediately
        if (i == 0) {
            percpu_setup_gdt(cpu);
            percpu_load_gdt(cpu);
            LOG_SERIAL("PERCPU", "BSP GDT/TSS loaded");
        }
    }
}

void percpu_init_ap(uint32_t cpu_index) {
    if (cpu_index >= ncpu || cpu_index == 0) {
        return;  // Invalid or BSP
    }
    
    struct percpu *cpu = &percpus[cpu_index];
    
    // Basic initialization (stacks already allocated)
    cpu->self = cpu;
    cpu->apic_id = get_apic_id();
    cpu->cpu_index = cpu_index;
    cpu->is_bsp = false;
    cpu->ncli = 0;
    cpu->intena = 0;
    cpu->current_thread = (void *)0;
    cpu->idle_thread = (void *)0;
    
    // Set up APIC ID mapping
    apic_to_cpu[cpu->apic_id] = cpu_index;
    
    // Set up TSS (stacks should already be allocated)
    setup_tss(cpu);
    
    // Set up and load this CPU's GDT
    percpu_setup_gdt(cpu);
    percpu_load_gdt(cpu);
    
    // Mark as started
    cpu->started = true;
}

void percpu_log_cpu_info(void) {
    LOG_SERIAL("PERCPU", "=== Per-CPU Data Summary ===");
    LOG_SERIAL("PERCPU", "Total CPUs: %d", ncpu);
    
    for (uint32_t i = 0; i < ncpu; i++) {
        struct percpu *cpu = &percpus[i];
        LOG_SERIAL("PERCPU", "CPU %d: APIC ID=%d, %s, started=%d",
                   cpu->cpu_index,
                   cpu->apic_id,
                   cpu->is_bsp ? "BSP" : "AP",
                   cpu->started);
        LOG_SERIAL("PERCPU", "  int_stack=%p, kstack=%p",
                   cpu->int_stack, cpu->kstack);
        LOG_SERIAL("PERCPU", "  TSS RSP0=%p, IST1=%p",
                   (void *)cpu->tss.rsp0, (void *)cpu->tss.ist1);
        LOG_SERIAL("PERCPU", "  GDT at %p",
                   cpu->gdt);
    }
    LOG_SERIAL("PERCPU", "============================");
}

void percpu_log_timer_ticks(void) {
    LOG_SERIAL("PERCPU", "=== Timer Interrupt Counts ===");
    for (uint32_t i = 0; i < ncpu; i++) {
        struct percpu *cpu = &percpus[i];
        LOG_SERIAL("PERCPU", "CPU %d (APIC %d): %llu ticks",
                   cpu->cpu_index,
                   cpu->apic_id,
                   cpu->timer_ticks);
    }
    LOG_SERIAL("PERCPU", "==============================");
}

// ============================================================================
// Interrupt State Management (moved from spinlock.c)
// ============================================================================

// Read EFLAGS register
static inline uint64_t read_eflags(void) {
    uint64_t eflags;
    asm volatile("pushfq; popq %0" : "=r"(eflags));
    return eflags;
}

// Interrupt flag in EFLAGS
#define FL_IF 0x00000200

void pushcli(void) {
    uint64_t eflags = read_eflags();
    cli();  // Disable interrupts
    
    struct percpu *cpu = mycpu();
    if (cpu->ncli == 0) {
        cpu->intena = eflags & FL_IF;
    }
    cpu->ncli++;
}

void popcli(void) {
    if (read_eflags() & FL_IF) {
        // Panic: popcli with interrupts enabled
        panic("popcli with interrupts enabled");
    }
    
    struct percpu *cpu = mycpu();
    if (--cpu->ncli < 0) {
        // Panic: unbalanced popcli
        panic("unbalanced popcli");
    }
    
    if (cpu->ncli == 0 && cpu->intena) {
        sti();  // Re-enable interrupts
    }
}
