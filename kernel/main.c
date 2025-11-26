//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//
// Main kernel entry point and thread utilities
//

#include "vga/vga.h"
#include "idt/idt.h"
#include "tty/tty.h"
#include "kalloc/kalloc.h"
#include "memlayout.h"
#include "lib/include/x86_64.h"
#include "paging/paging.h"
#include "sched/proc.h"
#include "sched/threads.h"
#include "sched/scheduler.h"
#include "kalloc/slab.h"
#include "kalloc/slob.h"
#include "kalloc/slub.h"


typedef void* (*alloc_func_t)(size_t size);

typedef void (*free_func_t)(void* ptr);

void test_allocator(alloc_func_t alloc, free_func_t free_mem, const char* name) {
    printf("=== Testing allocator: %s ===\n", name);

    void *b1 = alloc(8);
    void *b2 = alloc(16);
    void *b3 = alloc(32);
    void *b4 = alloc(64);

    printf("Allocating 8-byte, 16-byte, 32-byte and 64-byte objects\n");
    printf("Allocated blocks: %p %p %p %p\n", b1, b2, b3, b4);

    printf("Freeing 16-byte and 64-byte objects\n");
    free_mem(b2);
    free_mem(b4);

    void *b5 = alloc(16);
    printf("Allocated another 16-byte object: %p\n", b5);

    void *b6 = alloc(13);
    printf("Allocated 13-byte object: %p\n", b6);

    printf("=== Finished testing %s ===\n\n", name);
}

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
    init_tty();
    
    for (uint8_t i=0; i < TERMINALS_NUMBER; i++) {
        set_tty(i);
        printf("TTY %d\n", i);
    }
    set_tty(0);
    printf(
    " CR3: %x\n", rcr3()
    );

    print("$ \n");
    printf("Kernel end at address: %d\n", KEND);
    printf("Kernel size: %d\n", KEND - KSTART);

    kinit(KEND, INIT_PHYSTOP);
    pagetable_t kernel_table = kvminit(INIT_PHYSTOP, PHYSTOP);
    printf("kernel table: %p\n", kernel_table);
    kinit(INIT_PHYSTOP, PHYSTOP);
    printf("Successfully allocated physical memory up to %p\n", PHYSTOP);
    printf("%d pages available in allocator\n", count_pages());

    struct proc_node *init_proc_node = procinit();
    printf("Init proc node %p\n", init_proc_node);
    struct thread *init_thread = peek_thread_list(init_proc_node->data->threads);
    printf("Got init thread\n");

    test_allocator(kmalloc_slab, kfree_slab, "Slab Allocator");

    test_allocator(slob_alloc, slob_free, "Slob Allocator");

    test_allocator(malloc_slub, free_slub, "Slub Allocator");

    setup_idt();

    // scheduler();

    while(1) {};
    return 0;
}
