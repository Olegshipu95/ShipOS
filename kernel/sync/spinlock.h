#ifndef UNTITLED_OS_SPINLOCK_H
#define UNTITLED_OS_SPINLOCK_H

#include <inttypes.h>
#include "../sched/proc.h"
#include "../lib/include/x86_64.h"

struct percpu; // Forward declaration

struct spinlock {
    uint32_t is_locked;
    const char *name;
    struct percpu *cpu;  // Track which CPU holds the lock
};

void init_spinlock(struct spinlock *lock, const char *name);
void acquire_spinlock(struct spinlock *lk);
void release_spinlock(struct spinlock *lk);
int holding_spinlock(struct spinlock *lock);

#endif