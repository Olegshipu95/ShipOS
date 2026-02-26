//
// Created by ShipOS developers on 03.01.24.
// Copyright (c) 2024-2026 SHIPOS. All rights reserved.
//
// Simple SMP-safe mutex implementation for ShipOS kernel.
//

#include "mutex.h"
#include "../sched/smp_sched.h"
#include "../sched/percpu.h"
#include "../lib/include/panic.h"

void init_mutex(struct mutex *m, const char *name) {
    init_spinlock(&m->lock, name);
    m->locked = 0;
    m->owner = (void *)0;
    m->name = name;
}

void acquire_mutex(struct mutex *m) {
    acquire_spinlock(&m->lock);
    
    // Sleep until the mutex is unlocked.
    // The while loop protects against spurious wakeups.
    while (m->locked) {
        sleep(m, &m->lock);
    }
    
    m->locked = 1;
    m->owner = curthread(); // curthread() is a macro from percpu.h
    
    release_spinlock(&m->lock);
}

void release_mutex(struct mutex *m) {
    acquire_spinlock(&m->lock);
    
    // Only the thread that acquired the mutex can release it
    if (!m->locked || m->owner != curthread()) {
        panic("release_mutex: not holding or not owner");
    }
    
    m->locked = 0;
    m->owner = (void *)0;
    
    // Wake up all threads sleeping on this mutex.
    // Only one will successfully acquire it because of the while loop in acquire_mutex.
    wakeup(m);
    
    release_spinlock(&m->lock);
}