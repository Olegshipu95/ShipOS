//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023-2026 SHIPOS. All rights reserved.
//

#ifndef UNTITLED_OS_SHEDULER_H
#define UNTITLED_OS_SHEDULER_H

#include "proc.h"
#include "threads.h"

#define ROUNDS_PER_PROC 5

// Forward declaration to avoid circular dependencies
struct spinlock;

extern struct spinlock sched_lock;

/**
 * @brief Initialize the scheduler and its locks
 */
void init_scheduler(void);

/**
 * @brief Get the next runnable thread
 * @return Pointer to the next thread to run
 */
struct thread *get_next_thread(void);

/**
 * @brief Main scheduler loop, runs on each CPU
 */
void scheduler(void);

/**
 * @brief Yield the CPU to the scheduler
 */
void yield(void);

#endif //UNTITLED_OS_SHEDULER_H