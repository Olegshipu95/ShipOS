//
// Created by ShipOS developers on 03.01.24.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//
// Mutex interface for ShipOS kernel
// Provides basic mutual exclusion with support for thread blocking via sleep/wakeup.
//

#ifndef UNTITLED_OS_MUTEX_H
#define UNTITLED_OS_MUTEX_H

#include "spinlock.h"
#include "../sched/threads.h"

/**
 * @brief Mutex structure
 * Uses a low-level spinlock to protect its state.
 * Threads attempting to acquire a locked mutex will be put to sleep
 * by the scheduler until it is released.
 */
struct mutex {
    struct spinlock lock;   // Protects the mutex state
    uint32_t locked;        // 1 if locked, 0 if unlocked
    struct thread *owner;   // The thread that currently holds the mutex
    const char *name;             // Name for debugging
};

/**
 * @brief Initialize a mutex
 * @param m Pointer to the mutex
 * @param name Name of the mutex
 */
void init_mutex(struct mutex *m, const char *name);

/**
 * @brief Acquire the mutex
 * Blocks the current thread if mutex is already held.
 * @param m Pointer to the mutex
 */
void acquire_mutex(struct mutex *m);

/**
 * @brief Release the mutex
 * Wakes up waiting threads if any.
 * @param m Pointer to the mutex
 */
void release_mutex(struct mutex *m);

#endif //UNTITLED_OS_MUTEX_H