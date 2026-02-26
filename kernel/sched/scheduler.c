//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023-2026 SHIPOS. All rights reserved.
//

#include "scheduler.h"
#include "proc.h"
#include "threads.h"
#include "../lib/include/x86_64.h"
#include "../lib/include/panic.h"
#include "../sync/spinlock.h"

// Global scheduler lock to protect process and thread lists
struct spinlock sched_lock;
uint32_t current_proc_rounds = 0;

void init_scheduler() {
    init_spinlock(&sched_lock, "sched_lock");
}

// Must be called with sched_lock held
struct thread *get_next_thread() {
    if (proc_list == 0) return 0; // Return 0 instead of panic for SMP idle

    struct proc *first_proc = peek_proc_list(proc_list);
    struct proc *current_proc;

    if (current_proc_rounds >= ROUNDS_PER_PROC) {
        current_proc_rounds = 0;
        shift_proc_list(&proc_list);
    }

    do {
        current_proc = peek_proc_list(proc_list);

        if (current_proc->threads != 0) {
            struct thread *first_thread = peek_thread_list(current_proc->threads);
            struct thread *current_thread_ptr;

            do {
                current_thread_ptr = peek_thread_list(current_proc->threads);
                shift_thread_list(&(current_proc->threads));
                if (current_thread_ptr->state == RUNNABLE) {
                    return current_thread_ptr;
                }
            } while (peek_thread_list(current_proc->threads) != first_thread);
        }

        current_proc_rounds = 0;
        shift_proc_list(&proc_list);
    } while (peek_proc_list(proc_list) != first_proc);

    return 0; // No runnable threads found
}

void scheduler() {
    while (1) {
        // Enable interrupts so this CPU can receive timer ticks and IPIs
        sti();

        acquire_spinlock(&sched_lock);

        struct thread *next_thread = get_next_thread();
        
        if (next_thread != 0) {
            current_cpu.current_thread = next_thread;
            // Switch context using the CPU's private scheduler context
            switch_context(&(current_cpu.scheduler_ctx), next_thread->context);
            
            // Re-enters here after a thread calls yield() or sleep()
            current_cpu.current_thread = 0;
            release_spinlock(&sched_lock);
        } else {
            // Nothing to run. Release lock and wait for an interrupt (idle)
            release_spinlock(&sched_lock);
            asm volatile("hlt");
        }
    }
}

void yield() {
    acquire_spinlock(&sched_lock);
    current_cpu.current_thread->state = RUNNABLE;
    switch_context(&(current_cpu.current_thread->context), current_cpu.scheduler_ctx);
    release_spinlock(&sched_lock);
}