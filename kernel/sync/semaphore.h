//
// Created by ShipOS developers.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//
// Semaphore interface for ShipOS kernel.
//

#ifndef UNTITLED_OS_SEMAPHORE_H
#define UNTITLED_OS_SEMAPHORE_H

#include "spinlock.h"

/**
 * @brief Counting semaphore structure
 */
struct semaphore {
    struct spinlock lock; // Protects the semaphore state
    int value;            // Current counter value
    const char *name;           // Name for debugging
};

void sem_init(struct semaphore *s, int value, const char *name);

void sem_wait(struct semaphore *s);

void sem_post(struct semaphore *s);

#endif //UNTITLED_OS_SEMAPHORE_H