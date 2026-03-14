//
// Created by ShipOS developers.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//

#ifndef UNTITLED_OS_BARRIER_H
#define UNTITLED_OS_BARRIER_H

#include "spinlock.h"

/**
 * @brief Synchronization barrier
 * Blocks all calling threads until a specified number of threads
 * have reached the barrier.
 */
struct barrier {
    uint32_t threshold;  // Number of threads required to trip the barrier
    uint32_t count;      // Number of threads currently waiting
    uint32_t generation; // Generation counter to allow barrier reuse
    struct spinlock lock;
};

void init_barrier(struct barrier *b, uint32_t count, const char *name);

/**
 * @brief Wait at the barrier
 * Blocks the current thread until all 'threshold' threads have called this function.
 */
void barrier_wait(struct barrier *b);

#endif // UNTITLED_OS_BARRIER_H