//
// Created by untitled-os developers on 03.01.24.
// Copyright (c) 2024 untitled-os. All rights reserved.
//
// Mutex interface for untitled-os kernel
// Provides basic mutual exclusion with support for thread blocking.
//

#ifndef UNTITLED_OS_MUTEX_H
#define UNTITLED_OS_MUTEX_H

#include "spinlock.h"
#include "../sched/threads.h"
#include "../kalloc/kalloc.h"
#include "../sched/scheduler.h"

/**
 * @brief Mutex structure
 * 
 * Combines a low-level spinlock with a waiting thread list.
 * Threads attempting to acquire a locked mutex will be blocked
 * and added to the thread_list.
 */
struct mutex {
    struct spinlock *spinlock;   ///< underlying spinlock protecting the mutex
    struct thread_node *thread_list; ///< linked list of threads waiting for this mutex
};

/**
 * @brief Initialize a mutex
 * @param lk Pointer to the mutex
 * @param name Name for internal spinlock (for debugging)
 * @return 0 on success (currently unused)
 */
int init_mutex(struct mutex *lk, char *name);

/**
 * @brief Acquire the mutex
 * Blocks the current thread if mutex is already held.
 * @param lk Pointer to the mutex
 */
void acquire_mutex(struct mutex *lk);

/**
 * @brief Release the mutex
 * Wakes up a waiting thread if any; otherwise, releases the spinlock.
 * @param lk Pointer to the mutex
 */
void release_mutex(struct mutex *lk);

#endif //UNTITLED_OS_MUTEX_H

