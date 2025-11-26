//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

#ifndef UNTITLED_OS_SPINLOCK_H
#define UNTITLED_OS_SPINLOCK_H

#include <inttypes.h>
#include "../lib/include/x86_64.h"

// Forward declaration to avoid circular dependency with proc.h
struct cpu;

struct spinlock {
    uint8_t is_locked;
    char *name;
};

void init_spinlock(struct spinlock *lock, char *name);

void acquire_spinlock(struct spinlock *lk);

void release_spinlock(struct spinlock *lk);

int holding_spinlock(struct spinlock *lock);

#endif //UNTITLED_OS_SPINLOCK_H
