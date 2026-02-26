//
// Created by ShipOS developers on 10.01.26.
// Copyright (c) 2026 SHIPOS. All rights reserved.
//
// SMP-aware scheduler for multi-core systems.
// Each CPU has its own run queue and schedules threads independently.
//

#ifndef SHIP_OS_SMP_SCHED_H
#define SHIP_OS_SMP_SCHED_H

#include "threads.h"
#include "percpu.h"
#include "../sync/spinlock.h"

// ============================================================================
// Scheduler Configuration
// ============================================================================

// Time slice for round-robin scheduling (number of timer ticks)
#define SCHED_TIME_SLICE 5

// Load balancing threshold - trigger migration if difference exceeds this
#define LOAD_BALANCE_THRESHOLD 2

// ============================================================================
// Global Scheduler State
// ============================================================================

// Global lock for operations that affect multiple CPUs (thread creation, migration)
extern struct spinlock sched_lock;

// Flag indicating if scheduler is fully initialized
extern bool sched_initialized;

// ============================================================================
// Per-CPU Scheduler Functions
// ============================================================================

/**
 * @brief Initialize the scheduler subsystem
 *
 * Must be called once by BSP before starting APs.
 * Sets up global scheduler state and BSP's scheduler.
 */
void sched_init(void);

/**
 * @brief Initialize scheduler for the current CPU
 *
 * Called by each CPU (BSP and APs) during initialization.
 * Sets up per-CPU run queue and idle thread.
 */
void sched_init_cpu(void);

/**
 * @brief Add a thread to a CPU's run queue
 *
 * If cpu_index is -1, the thread is added to the least loaded CPU.
 * Otherwise, adds to the specified CPU's run queue.
 *
 * @param thread Thread to add
 * @param cpu_index Target CPU index, or -1 for automatic selection
 */
void sched_add_thread(struct thread *thread, int cpu_index);

/**
 * @brief Get the next runnable thread for the current CPU
 *
 * Returns the next thread from the current CPU's run queue.
 * If no threads are available, returns the idle thread.
 *
 * @return Next thread to run
 */
struct thread *sched_get_next(void);

/**
 * @brief Remove a thread from its CPU's run queue
 *
 * @param thread Thread to remove
 */
void sched_remove_thread(struct thread *thread);

/**
 * @brief Yield the current thread's time slice
 *
 * Saves current thread context and switches to scheduler.
 */
void sched_yield(void);

/**
 * @brief Exit the current thread
 *
 * Removes thread from run queue and switches to scheduler.
 * The thread will never run again.
 */
void sched_exit(void);

/**
 * @brief Main scheduler loop for a CPU
 *
 * Never returns. Continuously picks threads and runs them.
 */
void sched_run(void);

/**
 * @brief Timer tick handler for scheduler
 *
 * Called from timer interrupt. Handles time slice expiration
 * and triggers context switch if needed.
 */
void sched_tick(void);

// ============================================================================
// Load Balancing (Optional)
// ============================================================================

/**
 * @brief Perform load balancing across CPUs
 *
 * Migrates threads from overloaded CPUs to underloaded ones.
 * Should be called periodically (e.g., every N timer ticks).
 */
void sched_balance(void);

/**
 * @brief Find the CPU with the least load
 *
 * @return CPU index of the least loaded CPU
 */
uint32_t sched_find_least_loaded(void);

// ============================================================================
// Debugging
// ============================================================================

/**
 * @brief Log scheduler state for all CPUs
 */
void sched_log_state(void);

#endif // SHIP_OS_SMP_SCHED_H
