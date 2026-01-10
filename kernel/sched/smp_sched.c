//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// SMP-aware scheduler implementation.
//

#include "smp_sched.h"
#include "percpu.h"
#include "threads.h"
#include "../lib/include/logging.h"
#include "../lib/include/panic.h"
#include "../lib/include/memset.h"
#include "../lib/include/x86_64.h"
#include "../kalloc/kalloc.h"

// ============================================================================
// Global State
// ============================================================================

struct spinlock sched_lock;
bool sched_initialized = false;

// ============================================================================
// Idle Thread
// ============================================================================

/**
 * @brief Yield from idle thread to check for work
 * 
 * Similar to sched_yield but specifically for idle thread.
 * Returns to scheduler context which will check for real work.
 */
static void idle_yield(void)
{
    struct percpu *cpu = mycpu();
    struct thread *idle = cpu->idle_thread;
    
    // Switch back to scheduler context
    switch_context(&idle->context, cpu->scheduler_ctx);
}

/**
 * @brief Idle thread function
 *
 * Runs when no other threads are available.
 * Uses HLT instruction to save power while waiting for interrupts.
 * After each interrupt (like timer), yields to scheduler to check for work.
 */
static void idle_thread_func(void *arg)
{
    (void) arg; // Unused
    while (1)
    {
        sti();               // Enable interrupts
        asm volatile("hlt"); // Wait for interrupt
        
        // After waking from hlt (e.g., timer interrupt), yield to 
        // let scheduler check if there's real work to do
        idle_yield();
    }
}

/**
 * @brief Create idle thread for a CPU
 */
static struct thread *create_idle_thread(void)
{
    struct thread *idle = create_thread(idle_thread_func, 0, 0);
    if (idle)
    {
        idle->state = RUNNABLE;
    }
    return idle;
}

// ============================================================================
// Run Queue Management (Per-CPU)
// ============================================================================

/**
 * @brief Add thread to current CPU's run queue (internal, no lock)
 */
static void runqueue_add_unlocked(struct percpu *cpu, struct thread *thread)
{
    push_thread_list(&cpu->run_queue, thread);
    cpu->num_threads++;
}

/**
 * @brief Remove thread from current CPU's run queue (internal)
 *
 * @return true if thread was found and removed
 */
static bool runqueue_remove_unlocked(struct percpu *cpu, struct thread *thread)
{
    if (cpu->run_queue == 0)
    {
        return false;
    }

    struct thread_node *start = cpu->run_queue;
    struct thread_node *current = start;

    do
    {
        if (current->data == thread)
        {
            // Found it - remove from circular list
            if (current->next == current)
            {
                // Only node in list
                cpu->run_queue = 0;
            }
            else
            {
                current->prev->next = current->next;
                current->next->prev = current->prev;
                if (cpu->run_queue == current)
                {
                    cpu->run_queue = current->next;
                }
            }
            kfree(current);
            cpu->num_threads--;
            return true;
        }
        current = current->next;
    } while (current != start);

    return false;
}

/**
 * @brief Get next runnable thread from queue (round-robin)
 */
static struct thread *runqueue_get_next(struct percpu *cpu)
{
    if (cpu->run_queue == 0)
    {
        return 0;
    }

    struct thread_node *start = cpu->run_queue;
    struct thread_node *current = start;

    do
    {
        struct thread *t = current->data;
        if (t->state == RUNNABLE)
        {
            // Move to next for round-robin
            cpu->run_queue = current->next;
            return t;
        }
        current = current->next;
    } while (current != start);

    return 0; // No runnable threads
}

// ============================================================================
// Scheduler Initialization
// ============================================================================

void sched_init(void)
{
    init_spinlock(&sched_lock, "sched");
    sched_initialized = true;
    LOG_SERIAL("SCHED", "SMP scheduler initialized");
}

void sched_init_cpu(void)
{
    struct percpu *cpu = mycpu();

    // Initialize run queue
    cpu->run_queue = 0;
    cpu->num_threads = 0;
    cpu->scheduler_ready = false;

    // Allocate scheduler context
    uint64_t sched_stack = (uint64_t) kalloc();
    if (sched_stack == 0)
    {
        panic("sched_init_cpu: failed to allocate scheduler stack");
    }
    memset((void *) sched_stack, 0, PGSIZE);

    // Set up scheduler context at top of stack
    sched_stack += PGSIZE;
    sched_stack -= sizeof(struct context);
    cpu->scheduler_ctx = (struct context *) sched_stack;
    memset(cpu->scheduler_ctx, 0, sizeof(struct context));

    // Create idle thread for this CPU
    cpu->idle_thread = create_idle_thread();
    if (cpu->idle_thread == 0)
    {
        panic("sched_init_cpu: failed to create idle thread");
    }

    cpu->current_thread = 0;

    LOG_SERIAL("SCHED", "CPU %d scheduler initialized (idle=%p)",
               cpu->cpu_index, cpu->idle_thread);
}

// ============================================================================
// Thread Management
// ============================================================================

void sched_add_thread(struct thread *thread, int cpu_index)
{
    if (thread == 0)
    {
        panic("sched_add_thread: null thread");
    }

    acquire_spinlock(&sched_lock);

    struct percpu *target_cpu;

    if (cpu_index < 0)
    {
        // Auto-select: find least loaded CPU
        target_cpu = &percpus[sched_find_least_loaded()];
    }
    else if ((uint32_t) cpu_index >= ncpu)
    {
        release_spinlock(&sched_lock);
        panic("sched_add_thread: invalid cpu_index");
    }
    else
    {
        target_cpu = &percpus[cpu_index];
    }

    thread->state = RUNNABLE;
    runqueue_add_unlocked(target_cpu, thread);

    LOG_SERIAL("SCHED", "Added thread %p to CPU %d (now has %d threads)",
               thread, target_cpu->cpu_index, target_cpu->num_threads);

    release_spinlock(&sched_lock);
}

void sched_remove_thread(struct thread *thread)
{
    if (thread == 0)
    {
        return;
    }

    acquire_spinlock(&sched_lock);

    // Search all CPUs for this thread
    for (uint32_t i = 0; i < ncpu; i++)
    {
        if (runqueue_remove_unlocked(&percpus[i], thread))
        {
            LOG_SERIAL("SCHED", "Removed thread %p from CPU %d", thread, i);
            break;
        }
    }

    release_spinlock(&sched_lock);
}

struct thread *sched_get_next(void)
{
    struct percpu *cpu = mycpu();

    // Try to get a runnable thread from our queue
    struct thread *next = runqueue_get_next(cpu);

    if (next != 0)
    {
        return next;
    }

    // No runnable threads - return idle thread
    return cpu->idle_thread;
}

// ============================================================================
// Context Switching
// ============================================================================

void sched_yield(void)
{
    struct percpu *cpu = mycpu();
    struct thread *current = cpu->current_thread;

    if (current == 0 || current == cpu->idle_thread)
    {
        return; // Nothing to yield from
    }

    // Mark as runnable (will be picked up again)
    current->state = RUNNABLE;

    // Switch to scheduler context
    switch_context(&current->context, cpu->scheduler_ctx);
}

/**
 * @brief Exit the current thread
 * 
 * Removes the current thread from the run queue and switches
 * to the scheduler. The thread will never run again.
 */
void sched_exit(void)
{
    struct percpu *cpu = mycpu();
    struct thread *current = cpu->current_thread;

    if (current == 0 || current == cpu->idle_thread)
    {
        return; // Can't exit idle thread
    }

    // Mark as exited
    current->state = EXIT;
    
    // Remove from run queue
    runqueue_remove_unlocked(cpu, current);
    cpu->current_thread = 0;

    LOG_SERIAL("SCHED", "Thread %p exited on CPU %d", current, cpu->cpu_index);

    // Switch to scheduler - never returns
    switch_context(&current->context, cpu->scheduler_ctx);
    
    // Should never reach here
    panic("sched_exit returned");
}

void sched_run(void)
{
    struct percpu *cpu = mycpu();

    cpu->scheduler_ready = true;
    LOG_SERIAL("SCHED", "CPU %d entering scheduler loop", cpu->cpu_index);

    while (1)
    {
        sti(); // Enable interrupts while looking for work

        struct thread *next = sched_get_next();

        if (next != 0)
        {
            // LOG_SERIAL("SCHED", "CPU %d switching to thread %p, context=%p, rip=%p", 
            //            cpu->cpu_index, next, next->context, 
            //            next->context ? next->context->rip : 0);
            cpu->current_thread = next;
            next->state = ON_CPU;
            switch_context(&cpu->scheduler_ctx, next->context);
        }
    }
}

void sched_tick(void)
{
    // NOTE: Context switching from interrupt handlers is unsafe because
    // switch_context uses 'ret' instead of 'iret', leaving the interrupt
    // frame on the stack and corrupting CPU state.
    //
    // This function only handles waking up from idle when new work is available.
    // Preemption must happen via cooperative sched_yield() from thread context.
    //
    // TODO: Implement proper trap-frame based preemption if needed.
    
    struct percpu *cpu = mycpu();

    if (!cpu->scheduler_ready)
    {
        return;
    }

    // For now, just track that timer is firing - no preemption from interrupt
    // The idle thread loop handles checking for new work after each HLT
}

// ============================================================================
// Load Balancing
// ============================================================================

uint32_t sched_find_least_loaded(void)
{
    uint32_t min_load = UINT32_MAX;
    uint32_t min_cpu = 0;

    for (uint32_t i = 0; i < ncpu; i++)
    {
        if (percpus[i].started && percpus[i].num_threads < min_load)
        {
            min_load = percpus[i].num_threads;
            min_cpu = i;
        }
    }

    return min_cpu;
}

void sched_balance(void)
{
    // Find max and min loaded CPUs
    uint32_t max_load = 0, min_load = UINT32_MAX;
    uint32_t max_cpu = 0, min_cpu = 0;

    for (uint32_t i = 0; i < ncpu; i++)
    {
        if (!percpus[i].started)
            continue;

        uint32_t load = percpus[i].num_threads;
        if (load > max_load)
        {
            max_load = load;
            max_cpu = i;
        }
        if (load < min_load)
        {
            min_load = load;
            min_cpu = i;
        }
    }

    // Check if balancing is needed
    if (max_load - min_load < LOAD_BALANCE_THRESHOLD)
    {
        return;
    }

    acquire_spinlock(&sched_lock);

    // Migrate one thread from max to min
    struct percpu *src = &percpus[max_cpu];
    struct percpu *dst = &percpus[min_cpu];

    if (src->run_queue != 0 && src->num_threads > 1)
    {
        // Find a migratable thread
        struct thread_node *start = src->run_queue;
        struct thread_node *current = start;

        do
        {
            struct thread *t = current->data;
            if (t != src->current_thread &&
                t != src->idle_thread &&
                t->state == RUNNABLE)
            {
                // Migrate this thread
                runqueue_remove_unlocked(src, t);
                runqueue_add_unlocked(dst, t);
                LOG_SERIAL("SCHED", "Migrated thread %p from CPU %d to CPU %d",
                           t, max_cpu, min_cpu);
                break;
            }
            current = current->next;
        } while (current != start);
    }

    release_spinlock(&sched_lock);
}

// ============================================================================
// Debugging
// ============================================================================

void sched_log_state(void)
{
    LOG_SERIAL("SCHED", "=== Scheduler State ===");

    for (uint32_t i = 0; i < ncpu; i++)
    {
        struct percpu *cpu = &percpus[i];
        if (!cpu->started)
            continue;

        LOG_SERIAL("SCHED", "CPU %d: %d threads, current=%p, ready=%d",
                   i, cpu->num_threads, cpu->current_thread, cpu->scheduler_ready);
    }

    LOG_SERIAL("SCHED", "=======================");
}
